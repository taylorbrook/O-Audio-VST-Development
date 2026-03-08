# DSP Agent Memory

## Learned Patterns
- O-Prism: Wavetable mipmap anti-aliasing (FFT -> zero high bins -> IFFT per octave level) is the industry standard approach used by Serum/Vital/Surge -- always prefer over runtime oversampling for polyphonic synths due to CPU scaling with voice count
- O-Prism: JUCE 8 StateVariableTPTFilter only supports LP/BP/HP natively; notch filter = LP output + HP output from the same SVF state (not a separate mode)
- O-Prism: For 24dB filter slopes, cascade two StateVariableTPTFilter instances; apply resonance to first stage only to prevent excessive resonance buildup and self-oscillation
- O-Prism: polyBLEP is near-zero-cost anti-aliasing for classic waveforms (saw/square/triangle) -- preferred for sub oscillators and CPU-constrained contexts
- O-Prism: For synthesizers, always set IS_SYNTH TRUE + NEEDS_MIDI_INPUT TRUE in CMakeLists.txt -- omitting causes silent plugin in DAW (host won't route MIDI)
- O-Prism: Wavetable memory is significant (~20MB per table with float mipmaps at 2048 samples/frame); use lazy loading for large factory libraries
- O-Texture: ANIRA builds as shared library (libanira.dylib) + pulls ONNX Runtime (libonnxruntime.dylib) -- must embed both in plugin bundle Contents/Frameworks/ with rpath @loader_path/../Frameworks and fix anira's own onnxruntime reference via install_name_tool
- General: JUCE 8 getLatencySamples() is NOT virtual -- cannot override. Use setLatencySamples(N) in prepareToPlay() to report latency; the non-virtual getter returns the stored value
- General: Audio clicks come from signal discontinuities (abrupt level changes) -- even stopping at zero crossing can click if the derivative is discontinuous. Always use fade-in/fade-out ramps (5-20ms) for voice start/stop
- General: FFT overlap-add requires COLA-compliant windows (e.g., Hann with 50% overlap) -- non-COLA windows cause periodic clicking at frame boundaries
- General: Parameter changes in processBlock must be smoothed (juce::SmoothedValue or linear interpolation per sample) to prevent zipper noise -- never apply raw parameter values directly to gain/frequency

- O-Gain: For LUFS 400ms overlapping blocks with 100ms hop, use 4 sub-block accumulators (ring buffer of 100ms power sums) and combine all 4 for each 400ms block -- avoids storing raw sample history
- O-Gain: juce::dsp::IIR::Coefficients<double> constructor takes (b0, b1, b2, a0, a1, a2) -- assign via ReferenceCountedObjectPtr to filter.coefficients (not dereference assignment)
- O-Gain: BS.1770 K-weight filter coefficients are ONLY published for 48kHz -- must pre-calculate for 44100/88200/96000 via bilinear transform from analog prototype
- O-Gain: EBU R128 dual-gate runs in power domain: absolute gate threshold = 10^((-70+0.691)/10), relative gate = absoluteGatedMeanPower * 0.1 (which is -10 LU in power)
- O-Gain: BallisticsFilter uses processSample(channel, value) for per-sample processing -- channel index is always 0 when using separate L/R filter instances

## Common Issues
- WebSearch returns outdated JUCE 6/7 docs; always verify JUCE API by reading local source at /Users/taylorbrook/JUCE/modules/
- Bilinear transform introduces frequency warping near Nyquist -- biquad EQ filters sound compressed at high frequencies without cramping compensation
- Buffer boundary clicks: state variables (filter history, oscillator phase) must persist between processBlock calls -- reinitializing per buffer causes clicks

## Last Updated
2026-03-07 (O-Gain Stage 2 DSP implementation)
