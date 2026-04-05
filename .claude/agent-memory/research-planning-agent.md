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

- Error resolved: {"parentUuid":"690cb47e-3db7-4f8f-93f5-b93f6ef12e01","isSidechain":true,"promptId":"7a00c0a2-35ea-46
- Error resolved: {"parentUuid":"f88251d3-4138-460e-889e-4d2f5b3258ab","isSidechain":true,"promptId":"7a00c0a2-35ea-46
- Error resolved: {"parentUuid":"ae7ddd90-dd6c-4e26-868a-70c1d0908052","isSidechain":true,"promptId":"7a00c0a2-35ea-46
- Error resolved: {"parentUuid":"3f20c288-0c8f-4ee3-82d0-7db4c7462943","isSidechain":true,"promptId":"7a00c0a2-35ea-46

- O-Bowed: For physical modeling bowed strings, the friction model is the highest-risk component -- always implement memoryless core tier first (STK-style bow table) and validate basic bowed sound before adding NR-solved enhanced/quality tiers
- O-Bowed: juce::dsp::DelayLine with Thiran interpolation provides built-in allpass fractional delay -- use this for waveguide strings (flat magnitude response, critical for bowed string harmonics)
- O-Bowed: When extensive pre-existing research exists (4 documents in research/), the research phase becomes primarily a synthesis and JUCE API mapping exercise rather than a full web-search cycle
- O-Bowed: For body resonator morphing, recalculate biquad coefficients from interpolated freq/Q/gain via makePeakFilter() rather than raw coefficient lerp -- raw lerp can produce unstable filters
- O-Bowed: O-Lyrica WaveguideString + HarpSynthVoice + SympatheticResonanceEngine are directly reusable architectural patterns for any waveguide-based physical model synth

- Error resolved: {"parentUuid":"21be624b-7d8e-4a38-a06f-c005e81a318b","isSidechain":true,"promptId":"85f17ffb-4012-47
- Error resolved: {"parentUuid":"4db04bfc-5bf8-4fcf-bf37-d3e12ba9d36a","isSidechain":true,"promptId":"85f17ffb-4012-47

- O-Wind: For jet-drive flute models, use tanh saturation over STK's cubic (x-x^3) -- tanh is bounded for all inputs, cubic diverges for |x|>1 requiring additional clamping
- O-Wind: Flute jet excitation is one-directional (reads bore output from PREVIOUS sample) -- no circular dependency, no iterative solver needed. This is the key simplification vs bowed string friction junctions.
- O-Wind: In flute physical models, the bore waveguide IS the body -- no separate body resonator needed (unlike bowed strings). Eliminates entire morphable biquad bank system.
- O-Wind: Use Lagrange3rd (not Thiran) for jet delay line because it's continuously modulated by embouchure. JUCE docs explicitly warn Thiran "is unsuitable for applications requiring fast delay modulation."
- O-Wind: Flute vibrato should modulate breath pressure (not pitch) -- produces correlated pitch+amplitude variation matching real flute physics. All serious implementations (SWAM, STK) use this approach.

## Common Issues
- WebSearch returns outdated JUCE 6 docs; always verify JUCE API by reading local JUCE source at /Users/taylorbrook/JUCE/modules/
- JUCE getLatencySamples() is NOT virtual in JUCE 8 -- must use setLatencySamples() instead

## Last Updated
2026-04-04 (auto write-back)