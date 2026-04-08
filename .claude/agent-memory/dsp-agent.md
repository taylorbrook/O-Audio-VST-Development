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

- O-Formant: Mipmapped glottal wavetable reuses O-Prism WavetableData/Generator pattern -- replace "frame" axis with "Rd step" axis, bilinear instead of trilinear interpolation (2 Rd x 2 mipmap levels instead of 2 frame x 2 mipmap x 2 sample)
- O-Formant: LF model Newton-Raphson solver for alpha benefits from numerical derivative (finite difference da=0.01) over analytical derivative -- the closed-form df/dalpha is complex and error-prone; numerical approach converges reliably in <50 iterations for all Rd values
- O-Formant: MPESynthesiserVoice::getCurrentlyPlayingNote() returns struct by value -- hoist out of per-sample loop since pitchbend state is constant within a single renderNextBlock call
- O-Formant: juce::dsp::IIR::ArrayCoefficients::makeBandPass returns std::array<float,6> with layout [b0,b1,b2,a0(=1),a1,a2] -- a0 is always 1.0 (pre-normalized)

- O-Bowed: Body resonator normalization: precompute linear gain sums per preset, interpolate sums alongside morph, divide by morphed sum to keep level consistent across material morphs
- O-Bowed: makePeakFilter gainFactor is LINEAR not dB -- always wrap in Decibels::decibelsToGain() before passing; forgetting this causes massive over-boost (e.g., 12.0 linear vs 3.98 from +12dB)
- O-Bowed: Allpass decorrelator for stereo width from mono: single IIR allpass on R channel creates frequency-dependent phase offset; M/S width=0 collapses perfectly to mono (no cancellation)

- O-Reed: Strategy C conical bore (spherical wave scaling) is cheaper than Strategy B (correction filter) -- 2 multiplications vs 4-5 filter ops per sample; round-trip scaling product = 1.0 always (energy neutral)
- O-Reed: One-sample delay in waveguide naturally decouples implicit reed-bore equation -- no Newton-Raphson needed; pop-before-push with previous sample's p_bore_minus makes junction fully explicit
- O-Reed: IIR::Filter default constructor initializes with passthrough Coefficients(1,0,1,0) with capacity>=8 -- subsequent *filter.coefficients = Coefficients(...) copies in-place without allocation (safe for audio thread)
- O-Reed: Physical Z_c values are huge (~2.67e6 for clarinet bore) -- normalize radiated output by reference impedance before applying output gain + tanh safety clip
- O-Reed: Separating processSample to return u_reed (flow) instead of p_bore_plus (wave) enables inserting mouthpiece chamber between reed and bore without changing reed physics
- O-Reed: Mouthpiece chamber symplectic Euler needs initializeState(p_mouth) on activation to prevent transient click from zero-state discontinuity -- smooth param detection with 20ms time constant

- O-Wind: Per-voice oversampling with feedback loops: upsample zeros to get block structure, run ENTIRE feedback loop at 2x rate inside oversampled loop, downsample output. Cannot partially oversample a feedback system.
- O-Wind: For jet-drive waveguide, one-sample feedback delay is natural (jet delay provides temporal separation) -- no iterative solver needed unlike bow-string friction junction
- O-Wind: SmoothedValue reset() must use internal oversampled rate when called per-oversampled-sample; otherwise ramp duration is 2x too long

- O-Bowed: MPESynthesiserVoice migration: remove canPlaySound/startNote/stopNote/pitchWheelMoved/controllerMoved, add noteStarted/noteStopped(bool)/notePitchbendChanged/notePressureChanged/noteTimbreChanged/noteKeyStateChanged. MPE has no Sound concept (remove addSound).
- O-Bowed: MPESynthesiser voice API differences: isActive() not isVoiceActive(), getVoice() returns MPESynthesiserVoice* not SynthesiserVoice*, enableLegacyMode(2) for standard MIDI keyboard compat
- O-Bowed: Per-voice oversampling block pattern: clear voiceBuffer -> AudioBlock -> getSubBlock(0, numSamples) -> processSamplesUp -> per-sample loop on oversampledBlock -> processSamplesDown -> mix to output. Prepare waveguide/bow/friction at sampleRate*2.
- O-Bowed: MPE pitch bend + TuningEngine: compute tuningEngine->getFrequency(midiNote) THEN multiply by pow(2, totalPitchbendInSemitones/12) -- never use MPE's pre-computed 12-TET frequency

## Common Issues
- WebSearch returns outdated JUCE 6/7 docs; always verify JUCE API by reading local source at /Users/taylorbrook/JUCE/modules/
- Bilinear transform introduces frequency warping near Nyquist -- biquad EQ filters sound compressed at high frequencies without cramping compensation
- Buffer boundary clicks: state variables (filter history, oscillator phase) must persist between processBlock calls -- reinitializing per buffer causes clicks

- O-Bowed: Split waveguide junction (readJunction/writeJunction) enables external friction dispatch without duplicating delay-line logic -- keep original processSample for backward compat (DroneStringEngine)
- O-Bowed: Elasto-plastic bristle z state needs denormal flush (|z| < 1e-15f -> 0) AND clamp (1.5*z_ba) to prevent runaway in high-force scenarios
- O-Bowed: Thermal friction exp() LUT (256 entries, T_glass to T_max range) is sufficient for smooth interpolation -- precompute in prepare(), zero runtime transcendental cost

- O-Reed: 5-segment bore waveguide with C-style arrays of DelayLine: use brace initializer `segDelay{ {2048}, {2048}, ... }` in constructor init list -- explicit constructor works with direct-init braces in C++17
- O-Reed: Keefe three-port tone hole as stateless real-valued scatter: `scatter = -strength/(1+strength)`, applied as `p_scattered = scatter * (fwd + bwd)`, fwd_out = fwd + p_scattered, bwd_out = bwd + p_scattered -- 3 muls + 3 adds per junction
- O-Reed: Per-sample embouchure modulation via lightweight setEmbouchure() setter avoids recomputing all reed params per sample -- only embouchure field needs updating for vibrato/subtone expression
- O-Reed: Runtime 2x/4x oversampling switching: use two Oversampling instances (factor=1, factor=2), detect param change per-block, re-prepare DSP at new oversampled rate. Avoids realloc per-note; only triggers on user dropdown change.
- O-Reed: Reset oversampling state (activeOS.reset()) on both noteStarted (normal onset) and noteStopped (immediate cutoff) to prevent stale filter state bleeding between notes

## Last Updated
2026-04-05 (O-Reed Phase 3.5 -- oversampling, MPE, tuning)
