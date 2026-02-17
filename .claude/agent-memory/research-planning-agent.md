# Research-Planning Agent Memory

## Learned Patterns
- O-Prism: Wavetable mipmap anti-aliasing (FFT -> zero bins -> IFFT per octave level) is the industry standard approach used by Serum/Vital/Surge -- always prefer over runtime oversampling for polyphonic synths due to CPU scaling
- O-Prism: JUCE 8 StateVariableTPTFilter only supports LP/BP/HP natively; notch filter must be implemented as LP+HP sum from the same SVF state
- O-Prism: For 24dB filter slopes, cascade two StateVariableTPTFilter instances; apply resonance to first stage only to prevent excessive resonance buildup
- O-Prism: The scala-tuning-engine module v2.1.0 exists at modules/tuning/ with full integration checklist -- always check for shared modules before assuming code needs to be written from scratch
- O-Prism: Complexity score caps at 5.0 but raw scores (O-Prism raw=14.0) should be documented to convey actual implementation weight
- O-Prism: For synthesizers, always use IS_SYNTH TRUE + NEEDS_MIDI_INPUT TRUE in CMakeLists.txt (juce8-critical-patterns.md #22) -- omitting causes silent plugin in DAW
- O-Prism: O-Lyrica provides a proven SynthesiserVoice pattern with TuningEngine pointer and APVTS pointer passed to each voice -- reuse this pattern for new synth plugins
- O-Prism: Wavetable memory is significant (~20MB per table with float mipmaps); need lazy loading strategy for large factory libraries
- O-Prism: polyBLEP is near-zero-cost anti-aliasing for classic waveforms (saw/square/triangle) -- preferred for sub oscillators

## Common Issues
- WebSearch returns outdated JUCE 6 docs; always verify JUCE API by reading local JUCE source at /Users/taylorbrook/JUCE/modules/
- JUCE getLatencySamples() is NOT virtual in JUCE 8 -- must use setLatencySamples() instead

## Last Updated
2026-02-16 (O-Prism Stage 0)
