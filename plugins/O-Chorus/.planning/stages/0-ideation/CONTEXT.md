# O-Chorus - Stage 0 Context

**Stage:** 0 (Ideation & Research)
**Status:** Complete
**Date:** 2026-02-07
**Complexity Score:** 2.8 (Moderate)
**Implementation Strategy:** Single-pass

---

## Research Findings

### Plugin Architecture Decision

**Chorus Engine Type:** Multi-voice BBD-style chorus (1-8 voices)

**Key architectural decisions made:**

1. **Lagrange3rd interpolation over Thiran allpass**
   - Rationale: Good quality/CPU balance for chorus (LFO rates <5 Hz)
   - Thiran offers flat magnitude response but 2x CPU cost
   - Lagrange3rd sufficient for slow modulation rates

2. **Mono sum input with stereo imaging via panning**
   - Prevents phase cancellation in mono sum
   - Simpler architecture than independent L/R processing
   - Stereo width comes from voice panning, not L/R modulation differences
   - Industry standard: Boss CE-1, Juno-60, Strymon Ola all use this approach

3. **Tanh saturation over WDF diode clipper**
   - 90% of BBD warmth at 10% of CPU cost
   - Smooth transfer function, minimal aliasing
   - Soft saturation appropriate for chorus (not heavy distortion)
   - No oversampling needed at subtle drive levels

4. **Fixed phase distribution over random**
   - Predictable, consistent sound across sessions
   - Even spacing ensures optimal LFO coverage
   - Symmetrical stereo image
   - Matches professional chorus implementations (Juno quadrature, tri-chorus)

### Professional Plugin Analysis

**Strymon Ola dBucket Chorus:**
- Tri-chorus architecture (3 delay lines)
- Phase distribution prevents "swoosh" and creates "cyclonic swirl"
- Multi-voice mode designed to "never become messy"

**Boss CE-1/CE-2 Chorus:**
- BBD chip: MN3207 (1024-stage)
- Natural high-frequency rolloff (~8kHz) from BBD bandwidth
- Warm saturation from bucket-brigade transfer characteristics

**Roland Juno-60:**
- BBD chip: MN3009 (256-stage)
- Quadrature LFO (90° phase offset between two chips)
- Legendary lush chorus sound

**D16 Syntorus 2:**
- "Analog BBD Emulation" mode adds warmth
- Per-voice depth variation for organic character

### DSP Algorithm Research

**Delay Line Interpolation:**
- Linear: Simple but introduces HF rolloff
- Lagrange 3rd: Better frequency response, moderate CPU
- Thiran Allpass: Flat magnitude, highest quality, 2x CPU
- **Choice:** Lagrange3rd (optimal for chorus use case)

**Source:** `research/delay-effects-comprehensive-guide.md`, JUCE documentation

**Analog Saturation:**
- Tanh: Smooth, musical saturation without harsh clipping
- Polynomial: Cheaper but less smooth
- WDF diode clipper: Most accurate but 10x CPU cost
- **Choice:** Tanh with asymmetry (mimics BBD transfer curve)

**Source:** `research/circuit-modeling-fundamentals.md`, web research on tape/BBD saturation

**Stereo Imaging:**
- Phase offset LFO: Can cause mono compatibility issues
- Quadrature LFO: Used by Juno-60 (2 chips with 90° offset)
- Equal-power panning: Industry standard, mono-safe
- **Choice:** Equal-power panning across voice array

**Source:** Web research on chorus stereo imaging, professional plugin analysis

### Parameter Range Research

**Rate (0.05 - 5.0 Hz):**
- Professional chorus typically 0.5-3 Hz
- Extended range allows ultra-slow (0.05 Hz) and fast vibrato (5 Hz)
- Logarithmic skew for control in musical range

**Depth (0 - 100%):**
- Maps to ±5ms modulation from 10ms base delay
- Full depth: 5-15ms delay range (classic chorus territory)
- Classic chorus: 30-60% depth

**Voices (1 - 8):**
- 1 voice: Simple vibrato
- 2 voices: Stereo chorus (Juno-style)
- 3 voices: Tri-chorus (Strymon-style)
- 4-8 voices: Lush ensemble
- Reference: Dimension D uses 4 BBD chips, Strymon Ola has tri-chorus mode

**Tone (-100% to +100%):**
- Neutral (0%): 8kHz lowpass (typical BBD rolloff)
- Dark (-100%): 2kHz (lo-fi BBD)
- Bright (+100%): 20kHz (modern, clean)
- BBD chips naturally roll off HF (MN3005/3207 ~8-10kHz)

### Complexity Assessment

**Tier 2 classification:**
- 6 parameters (moderate count)
- Standard DSP algorithms (delay, filter, saturation)
- Time-domain processing (no FFT)
- No file I/O, multi-output (>2ch), or complex MIDI

**Complexity score: 2.8**
- Parameter score: 1.2 (6 params)
- Algorithm count: 1 (modulated delay as single algorithm type)
- Feature count: 0.6 (modulation is standard for chorus)
- Total: 2.8 (below 3.0 threshold for staged implementation)

**Implementation strategy:** Single-pass
- Each stage completed in one focused session
- No phase breakdown within stages (DSP not complex enough to warrant)
- Straightforward integration (no exotic dependencies)

### JUCE API Mapping

**Core DSP components:**
- `juce::dsp::DelayLine<float, DelayLineInterpolationTypes::Lagrange3rd>` - Delay lines
- `juce::dsp::IIR::Filter<float>` - Tone control (one-pole lowpass)
- `juce::AudioProcessorValueTreeState` - Parameter management
- `juce::SmoothedValue<float>` - Parameter smoothing

**Required modules:**
- `juce_audio_processors` - AudioProcessor base, APVTS
- `juce_dsp` - DelayLine, IIR filters, ProcessSpec
- `juce_core` - Math, random number generation

**No custom implementations needed** - all components available in JUCE 8.

### Implementation Risks Identified

**HIGH Risk: Delay line modulation artifacts**
- Rapid delay changes can cause clicks even with interpolation
- Mitigation: Lagrange3rd interpolation, 5 Hz max rate, parameter smoothing
- Fallback: Switch to Thiran interpolation if artifacts persist

**MEDIUM Risk: CPU usage with 8 voices**
- 8 voices × DSP chain may exceed budget on low-end systems
- Mitigation: Optimize saturation (lookup table), SIMD, block processing
- Fallback: Reduce max voices to 4, offer "Lite" mode

**MEDIUM Risk: Stereo phase coherence**
- Independent modulation can cause mono compatibility issues
- Mitigation: Mono sum input, stereo from panning only
- Fallback: "Mono Safe" mode if compatibility critical

**LOW Risk: Parameter smoothing latency**
- 50-100ms smoothing may feel sluggish
- Mitigation: 50ms smoothing (fast enough for real-time)
- Fallback: Reduce to 20ms for "Fast" mode

**LOW Risk: Denormal numbers**
- Delay lines can accumulate denormals when idle
- Mitigation: ScopedNoDenormals, DC blocker, flush on silence
- Fallback: Add tiny noise if denormals persist

---

## Constraints and Decisions

### Technical Constraints

1. **Sample rate independence:** All time-based values scaled by sample rate
2. **Zero latency:** Chorus doesn't introduce processing latency (only wet delay)
3. **Thread safety:** APVTS for parameter sync, no custom locks needed
4. **Mono compatibility:** Mono sum input prevents phase cancellation

### Design Constraints

1. **Lush analog character:** BBD-inspired warmth via saturation + tone control
2. **Scalable voice count:** 1-8 voices for flexibility (vibrato to dense ensemble)
3. **Stereo imaging control:** Width parameter 0-100% for mix compatibility
4. **CPU efficiency:** Target <10% CPU with 8 voices at 48kHz

### User Experience Constraints

1. **Parameter count:** 6 parameters (focused, not overwhelming)
2. **Intuitive controls:** Logical grouping (Modulation, Character)
3. **Visual feedback:** LFO indicator shows modulation movement
4. **Preset compatibility:** Fixed phase distribution ensures repeatable sound

---

## Next Steps

### Stage 1: Foundation (Estimated 30 minutes)

**Priority tasks:**
1. CMakeLists.txt configuration (JUCE 8 patterns)
2. PluginProcessor skeleton (BusesProperties, basic lifecycle)
3. APVTS parameter definition (6 parameters with correct ranges)
4. State management (save/load)

**Success criteria:**
- Plugin builds and loads in DAW
- 6 parameters visible in automation
- Dry signal passes through

### Stage 2: DSP Implementation (Estimated 60 minutes)

**Priority tasks:**
1. Voice structure with DelayLine array
2. LFO with phase distribution and depth randomization
3. Per-voice processing (delay, saturation, tone, pan)
4. Mix stage and denormal prevention

**Success criteria:**
- Chorus effect audible
- All parameters functional
- No artifacts (clicks, pops)
- Mono compatibility verified

### Stage 3: GUI Implementation (Estimated 45 minutes)

**Priority tasks:**
1. WebView UI with 6 knobs
2. Parameter binding (JUCE 8 patterns)
3. LFO visual indicator
4. ES6 module integration

**Success criteria:**
- UI loads correctly
- Parameters bidirectionally synced
- LFO animation smooth

### Stage 4: Testing & Polish (Estimated 30 minutes)

**Priority tasks:**
1. Comprehensive functional testing
2. CPU profiling and optimization
3. Bug fixes
4. Documentation and presets

**Success criteria:**
- AU/VST3 validation passed
- CPU <10% (8 voices, 48kHz)
- No memory leaks
- Presets created

---

## Research Resources Consulted

**Web research:**
- Bucket-brigade delay history and emulation
- Multi-voice chorus phase distribution
- Analog saturation DSP techniques
- Stereo imaging for modulation effects

**Internal research documents:**
- `research/delay-effects-comprehensive-guide.md` - Interpolation methods, modulation
- `research/circuit-modeling-fundamentals.md` - Saturation, waveshaping, BBD modeling

**JUCE documentation:**
- DelayLine API and interpolation types
- dsp::IIR::Filter usage
- ProcessSpec and modern DSP pipeline

**Professional plugin analysis:**
- Strymon Ola (tri-chorus architecture)
- Boss CE-1/CE-2 (BBD characteristics)
- Roland Juno-60 (quadrature LFO)
- D16 Syntorus 2 (analog emulation)

---

## Handoff to Stage 1

**Complete artifacts:**
- ✅ ARCHITECTURE.md (full DSP specification)
- ✅ ROADMAP.md (complexity assessment, stage breakdown)
- ✅ CONTEXT.md (this document - research findings)

**Ready for implementation:**
- All DSP algorithms specified with JUCE API mappings
- Parameter ranges researched and justified
- Implementation risks identified with fallback plans
- Complexity assessed (2.8, single-pass strategy)
- Timeline estimated (~3.25 hours total)

**No blockers identified** - proceed to Stage 1 (Foundation).

---

**End of Stage 0 Context**
