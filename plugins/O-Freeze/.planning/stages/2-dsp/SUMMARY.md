# Stage 2: DSP - Execution Summary

## Completed: 2026-02-01

## Implementation Overview

Successfully implemented a granular freeze effect with three phases:

### Phase A: Simple Buffer Loop (Tasks 1-5)
- Added circular freeze buffer (2 seconds @ 192kHz max = 384,000 samples)
- Implemented buffer write/read with wraparound indexing
- Added crossfade envelope (50ms fade-in, 100ms fade-out) using `juce::LinearSmoothedValue`
- Integrated `juce::dsp::DryWetMixer` for MIX parameter

### Phase B: Threshold Gate (Tasks 6-7)
- Added RMS level detection with 20ms rolling window
- Implemented gate state machine (`GateState::Idle`, `GateState::Frozen`)
- 3dB hysteresis prevents flutter when signal hovers near threshold
- MODE parameter switches between Manual (button) and Threshold (automatic) triggering

### Phase C: Granular Engine (Tasks 8-11)
- Implemented 8-grain granular synthesis with 87.5% overlap
- Pre-computed asymmetric Blackman-Harris window (200ms grains) in `prepareToPlay()`
  - Extended attack (60%) for softer grain onsets
  - Compressed release (40%) for tighter fade-out
- Grain triggering with round-robin allocation
- DRIFT parameter controls grain position randomization (0-100%, default 25%)
- Overlap-add synthesis with normalization prevents clipping
- Sample-by-sample processing ensures correct stereo operation

## Files Modified

| File | Changes |
|------|---------|
| `Source/PluginProcessor.h` | DSP members, Grain struct, GateState enum, granular state |
| `Source/PluginProcessor.cpp` | prepareToPlay(), processBlock() with complete granular engine |
| `CMakeLists.txt` | Added `juce::juce_dsp` module link |

## DSP Components

- `juce::AudioBuffer<float> freezeBuffer` - Circular buffer for captured audio
- `juce::LinearSmoothedValue<float> freezeGain` - Crossfade envelope
- `juce::dsp::DryWetMixer<float> dryWetMixer` - Dry/wet mixing
- `std::array<Grain, 8> grains` - 8 simultaneous grains
- `std::vector<float> hannWindow` - Pre-computed asymmetric Blackman-Harris window
- `std::vector<float> rmsBuffer` - Rolling RMS detection window

## Parameter Connections

| Parameter | DSP Function |
|-----------|--------------|
| FREEZE | Manual trigger (MODE=Manual) - locks buffer write position |
| THRESHOLD | Auto-freeze level (MODE=Threshold) - triggers when RMS drops below |
| MODE | Selects Manual or Threshold triggering |
| DRIFT | Grain position randomization (0%=static, 100%=evolving texture) |
| MIX | Dry/wet blend via DryWetMixer |

## Real-Time Safety

- All buffers pre-allocated in `prepareToPlay()`
- Zero allocations in `processBlock()`
- `ScopedNoDenormals` present
- Atomic parameter reads via `getRawParameterValue()`
- Lock-free operation
- Bounded execution time

## Bug Fixes During Verification

Three issues found and fixed during DAW testing:

1. **Silence on freeze engage** - Grains weren't triggered immediately; added instant 8-grain activation with staggered positions on freeze engage

2. **Grain read position error** - Grains were reading ahead of writePosition (zeros) instead of behind it (captured audio); fixed to read from `writePosition - grainSize`

3. **Stereo processing bug** - Loop structure (channel→sample) caused channel 1 to read wrong positions; restructured to (sample→channel) with shared grain state

## Refinements

- Switched from Hann to asymmetric Blackman-Harris window for softer grain onsets
- Extended attack phase (60%) for gentler fade-in
- Changed DRIFT default from 0% to 25% for subtle movement out of the box

## Validation Results

- Build: PASSED
- pluginval: PASSED (strictness level 5)
- DAW testing: PASSED (mono and stereo)
- Installed to system plugin folders

## Known Limitations

- Fixed 2-second buffer (not user-adjustable)
- Fixed 200ms grain size (could be parameterized in future)
- Mono RMS detection (uses left channel only for stereo input)

## Next Steps

- **Stage 3: GUI** - WebView UI with freeze button, threshold slider, mode toggle, drift control, and mix knob
- Test in DAW with various audio sources
- Consider adding buffer length parameter in future version
