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


- O-Formant: juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(sampleRate, freq, Q) returns std::array<float,6> -- use for custom biquad coefficient computation while keeping state management lightweight
- O-Formant: For polyphonic synths with many filter instances (5 x 16 = 80), use custom biquad structs (32 bytes each) instead of juce::dsp::IIR::Filter -- ProcessSpec/AudioBlock overhead unacceptable at that scale
- O-Formant: juce::MPESynthesiserVoice has 6 pure virtual methods: noteStarted(), noteStopped(bool), notePressureChanged(), notePitchbendChanged(), noteTimbreChanged(), renderNextBlock() -- plus noteKeyStateChanged()
- O-Formant: enableLegacyMode() is on MPESynthesiserBase (which MPESynthesiser extends), not on MPESynthesiser directly -- signature: enableLegacyMode(int pitchbendRange = 2, Range<int> channelRange = Range<int>(1, 17))
- O-Formant: For source-filter vocal synths, Shepard interpolation (modified IDW) with tunable power beats barycentric/RBF when you have only 5 anchor points -- power parameter doubles as a musical "Focus" control
- O-Formant: When research docs and parameter-spec disagree on parameter count (22 vs 21), always follow the parameter-spec as the contract -- research docs are informational, not authoritative


- O-Bowed: For physical modeling bowed strings, the friction model is the highest-risk component -- always implement memoryless core tier first (STK-style bow table) and validate basic bowed sound before adding NR-solved enhanced/quality tiers
- O-Bowed: juce::dsp::DelayLine with Thiran interpolation provides built-in allpass fractional delay -- use this for waveguide strings (flat magnitude response, critical for bowed string harmonics)
- O-Bowed: When extensive pre-existing research exists (4 documents in research/), the research phase becomes primarily a synthesis and JUCE API mapping exercise rather than a full web-search cycle
- O-Bowed: For body resonator morphing, recalculate biquad coefficients from interpolated freq/Q/gain via makePeakFilter() rather than raw coefficient lerp -- raw lerp can produce unstable filters
- O-Bowed: O-Lyrica WaveguideString + HarpSynthVoice + SympatheticResonanceEngine are directly reusable architectural patterns for any waveguide-based physical model synth


- O-Wind: For jet-drive flute models, use tanh saturation over STK's cubic (x-x^3) -- tanh is bounded for all inputs, cubic diverges for |x|>1 requiring additional clamping
- O-Wind: Flute jet excitation is one-directional (reads bore output from PREVIOUS sample) -- no circular dependency, no iterative solver needed. This is the key simplification vs bowed string friction junctions.
- O-Wind: In flute physical models, the bore waveguide IS the body -- no separate body resonator needed (unlike bowed strings). Eliminates entire morphable biquad bank system.
- O-Wind: Use Lagrange3rd (not Thiran) for jet delay line because it's continuously modulated by embouchure. JUCE docs explicitly warn Thiran "is unsuitable for applications requiring fast delay modulation."
- O-Wind: Flute vibrato should modulate breath pressure (not pitch) -- produces correlated pitch+amplitude variation matching real flute physics. All serious implementations (SWAM, STK) use this approach.


- O-Reed: Guillemain Psi parameter (confined jet loss) is a single-term denominator addition to the Bernoulli flow equation -- computationally trivial but acoustically transformative for single-to-double-reed morphing
- O-Reed: For reed wind PM, use Strategy B (cylindrical waveguide + conical correction filter) initially -- Strategy C (true conical sections) has junction instabilities at taper transitions that add risk without proportional quality gain for v1.0
- O-Reed: Static reed table (STK-style piecewise linear with clamp) should always be Phase 1 -- validates bore waveguide independently before adding dynamic reed ODE complexity
- O-Reed: sqrt() in Bernoulli flow requires epsilon guard: sqrt(max(|dp|, 1e-10)) -- zero pressure difference causes denormals and NaN propagation
- O-Reed: Tone hole scattering junctions cost ~5-10 multiplies each per sample -- use 4 virtual holes + register hole (not full 24-hole clarinet lattice) for production quality at acceptable CPU



- O-Bassoon: For polyphonic modal synth voices (16 modes x 8 voices = 128 biquads), always use custom direct-form biquad arrays NOT juce::dsp::IIR::Filter -- ProcessSpec/AudioBlock overhead is unacceptable at scale (matches O-Formant's filter-bank precedent)
- O-Bassoon: Modal synthesis is a clean architectural choice for sustained-tone instruments -- no feedback loop, no oversampling, no iterative solver; CPU and stability are predictable, much simpler than waveguide approaches
- O-Bassoon: Bassoon's first formant at ~450-500 Hz is the dominant spectral feature -- implement per-mode amplitude weighting via Gaussian centered on FORMANT_F1 = 475 Hz; the strongest perceived partial is whichever harmonic falls closest to the formant peak, not the fundamental
- O-Bassoon: For modal-synth coefficient updates with vibrato, block-rate (not per-sample) coefficient recomputation is sufficient for vibrato rates up to 10 Hz at 256-sample buffers (~188 Hz update rate >> 10 Hz vibrato)
- O-Bassoon: When the brief excludes one plugin (here: O-Reed) as a dependency, document this as a Stage 1 acceptance test (verify via grep -rn for the excluded plugin name in CMake + sources)
- O-Bassoon: Wiring TuningEngine in headless at v1.0 (no UI, 12-TET default) costs ~50 lines but eliminates an invasive v1.1 refactor -- always favor seam-first when shared modules exist

- O-simpleFM: For FM synths ALWAYS spec phase modulation (PM) not true FM -- PM adds the modulator after phase integration so DC offset (feedback/asymmetry) becomes a fixed inaudible phase offset, not integrated pitch drift. Every commercial "FM" synth (DX7/Operator/FM8) is PM. Commit to ONE phase convention (radians, 1:1 with Bessel math); never mix with normalized-turns.
- O-simpleFM: Pedagogically transparent modulation index = raw radian index I (the Bessel argument), 0-20 perceptual taper. Carrier-null J0 zeros at I~=2.405/5.520/8.654 are marquee teaching annotations ("fundamental vanishes").
- O-simpleFM: DX7 self-feedback REQUIRES two-sample average (Tomisawa anti-hunting, US Pat 4,249,447) -- single-sample feedback hunts into a Nyquist limit-cycle screech. Clamp the feedback HISTORY (not just output), NaN-scrub at source, reset history on note-on. Max coeff ~= pi rad with x^1.5 taper.
- O-simpleFM: PolyBLEP/BLIT do NOT compose with hard FM (correction assumes steady phase increment). FM anti-aliasing = key-tracked index ceiling (Carson: (0.9*Nyq-fc)/fm - 1) + oversampling. polyBLEP is still fine for classic-waveform sub-oscillators, just not FM operators.
- O-simpleFM: Real-time-safe spectrum/scope = audio thread copy-only into pre-allocated juce::AbstractFifo ring; FFT on message-thread editor Timer (30 Hz). performFrequencyOnlyForwardTransform overwrites its buffer IN PLACE -- copy the scope window BEFORE the FFT. 4096-pt FFT + Blackman-Harris separates discrete sidebands clearly (Hann is blurrier).
- O-simpleFM: juce::dsp::LookupTableTransform sine needs explicit floor-modulo phase wrap (phase -= twoPi*floor(phase/twoPi)) before lookup -- the PM argument swings to many multiples of 2pi at high index. 1024 pts linear interp ~= 97 dB SNR (cubic unnecessary).
- O-simpleFM: Mod-env->index should be MULTIPLICATIVE (I_inst = base*((1-depth)+depth*modEnv)) default depth 1.0 -- at depth 1.0 + sustain 0 the tail is pure sine (carrier null reachable). Additive default muddies the mental model. Voice lifetime gates on AMP envelope only (long mod release must not keep silent voice alive).
- GENERAL: When Stage 0 deliverables already exist from a prior /plugin-research+plan pass, VERIFY their JUCE-class claims against local source (/Users/taylorbrook/JUCE/modules/) rather than regenerating, then finalize STATUS.md (often still says stage:ideation) + commit. Stage untracked .planning files from ideation together so the full contract set lands in git.
- GENERAL: research/ docs REQUIRE the 10-field YAML frontmatter or they're invisible to the resource manifest. Upstream docs created by a /plugin-research command often lack it -- add frontmatter when folding them into Stage 0. PLUGINS.md Stage-0 plugins use only the registry-table row (no per-plugin ### entry) -- don't fabricate one.

## Common Issues
- WebSearch returns outdated JUCE 6 docs; always verify JUCE API by reading local JUCE source at /Users/taylorbrook/JUCE/modules/
- JUCE getLatencySamples() is NOT virtual in JUCE 8 -- must use setLatencySamples() instead

## Last Updated
2026-04-26 (auto write-back)
