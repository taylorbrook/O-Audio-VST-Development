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

- O-Gain: juce::dsp::IIR::Coefficients<double> supports raw coefficient constructor (b0,b1,b2,a0,a1,a2) -- use this for ITU-R BS.1770 K-weighting filters instead of factory methods
- O-Gain: juce::dsp::BallisticsFilter has setAttackTime/setReleaseTime in ms and supports RMS level calculation type -- suitable for VU meter ballistics (300ms attack/release)
- O-Gain: For measurement-only subsystems (LUFS, true peak), use double precision IIR filters to prevent numerical drift during long accumulations (10-30s Learn sessions)
- O-Gain: BS.1770 K-weight coefficients are published only for 48kHz; pre-calculate for common sample rates (44100, 48000, 88200, 96000) as a lookup table rather than runtime bilinear transform
- O-Gain: juce::dsp::Oversampling adds latency to the signal path -- do NOT use for true peak detection in zero-latency plugins; use custom polyphase FIR instead (measurement side-chain only)

- \n   120→> \"Thanks to the fact that streaming services like Spotify and Apple Music are now normalizing songs so that the level is the same from tune to tune, there's no real benefit for compressing 
- Error resolved: {"parentUuid":"ba734643-0e49-44b1-a285-b3be4854ed55","isSidechain":true,"userType":"external","cwd":

- O-Formant: juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(sampleRate, freq, Q) returns std::array<float,6> -- use for custom biquad coefficient computation while keeping state management lightweight
- O-Formant: For polyphonic synths with many filter instances (5 x 16 = 80), use custom biquad structs (32 bytes each) instead of juce::dsp::IIR::Filter -- ProcessSpec/AudioBlock overhead unacceptable at that scale
- O-Formant: juce::MPESynthesiserVoice has 6 pure virtual methods: noteStarted(), noteStopped(bool), notePressureChanged(), notePitchbendChanged(), noteTimbreChanged(), renderNextBlock() -- plus noteKeyStateChanged()
- O-Formant: enableLegacyMode() is on MPESynthesiserBase (which MPESynthesiser extends), not on MPESynthesiser directly -- signature: enableLegacyMode(int pitchbendRange = 2, Range<int> channelRange = Range<int>(1, 17))
- O-Formant: For source-filter vocal synths, Shepard interpolation (modified IDW) with tunable power beats barycentric/RBF when you have only 5 anchor points -- power parameter doubles as a musical "Focus" control
- O-Formant: When research docs and parameter-spec disagree on parameter count (22 vs 21), always follow the parameter-spec as the contract -- research docs are informational, not authoritative

## Common Issues
- WebSearch returns outdated JUCE 6 docs; always verify JUCE API by reading local JUCE source at /Users/taylorbrook/JUCE/modules/
- JUCE getLatencySamples() is NOT virtual in JUCE 8 -- must use setLatencySamples() instead

## Last Updated
2026-04-04 (auto write-back)