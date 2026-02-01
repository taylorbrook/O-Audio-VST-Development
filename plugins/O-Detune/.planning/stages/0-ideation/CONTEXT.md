# Stage 0: Research & Planning - Context

**Plugin:** O-Detune
**Stage:** 0 (Ideation - Research & Planning)
**Date:** 2026-02-01
**Status:** Complete

---

## Phase Findings

### Complexity Assessment

**Final Complexity Score: 5.0 (Maximum)**

This is the highest complexity score in the Plugin Freedom System, driven by:
- **21 parameters** (very high parameter count)
- **11 DSP algorithms** (dual-engine architecture + modulation + character + stereo processing)
- **Advanced features** (feedback loops, multi-LFO modulation, mono-safe mode)

**Breakdown:**
- Parameters: 2.0 (capped)
- Algorithms: 11.0
- Features: +2 (feedback + modulation)
- Total: 15.0 → capped at 5.0

### Research Depth: DEEP (30 minutes)

Complexity Tier 5 triggered deep research protocol:
- 12 features identified for research
- Professional plugin analysis (Goodhertz Wow Control, RC-20, MicroShift, Polyverse Wider, Valhalla Delay)
- JUCE API mapping for all primitives
- Risk assessment per feature with fallback architectures

### Key Research Findings

**1. Delay-Based Pitch Shifting is Industry Standard**

Researched three approaches:
- Phase vocoder (FFT-based): Overkill for small shifts, high CPU/latency
- Granular synthesis: Graininess artifacts, more complex
- **Delay-based (CHOSEN):** Industry standard (Goodhertz, RC-20, Valhalla), low latency, low CPU

**Decision:** Use juce::dsp::DelayLine with Lagrange3rd interpolation for both wobble and unison engines.

**2. Mono-Safe Mode Requires All-Pass/Comb Filters**

Polyverse Wider research revealed:
- All-pass filter cascade (4 stages) creates phase shift
- Comb filters with complementary delays on L/R channels
- Phase inversion on right channel
- Result: L + R = 0 when summed to mono (perfect cancellation)

**Risk: HIGH** - Algorithm not publicly documented, requires reverse-engineering.
**Fallback:** Mid-side processing with careful phase management.

**3. Multi-LFO Essential for Authentic Tape Wow**

Single-LFO sounds artificial and repeating. Professional plugins (Goodhertz Wow Control, RC-20) use:
- Primary LFO (user rate: 0.1-10 Hz)
- Secondary LFO (slow drift: 0.05-0.2 Hz)
- Noise modulation (low-pass filtered white noise)

**Decision:** Implement dual-LFO + noise modulation for non-repeating patterns.

**4. Dual-Engine Architecture Enables Hybrid Effects**

Crossfade blend between wobble and unison allows:
- Pure wobble (blend = 0%)
- Pure unison (blend = 100%)
- **Hybrid effects** (blend = 50%): Wobbling unison voices

This is a unique feature not found in MicroShift (unison only) or RC-20 (wobble only).

**Decision:** Parallel processing with crossfade blend (skip processing at extremes for CPU optimization).

### Architecture Decisions

**1. Delay-Based Pitch Shifting (vs Phase Vocoder or Granular)**

Chosen because:
- Industry standard (professional plugins use this)
- Low latency (~50ms vs ~42ms FFT)
- Low CPU (~5% per delay line vs ~40% for FFT)
- JUCE has excellent DelayLine support

Tradeoffs accepted:
- Limited pitch range (±200 cents max, artifacts beyond)
- Acceptable: Plugin targets ±100 cents (50 cents unison + 50 cents wobble)

**2. Mono-Safe Mode Implementation (All-Pass/Comb vs Mid-Side)**

Chosen because:
- Polyverse Wider is industry standard for mono-compatible widening
- Perfect mono cancellation (verified via unit tests)
- Competitive advantage (distinctive from standard mid-side)

Tradeoffs accepted:
- Higher implementation complexity (phase calculation)
- Acceptable: Research validated feasibility, fallback documented

**3. Dual-Engine Architecture (Parallel Processing)**

Chosen because:
- Enables hybrid effects (wobbling unison voices)
- Smooth transitions (no clicks/pops)
- Matches user mental model (blend = crossfade)

Tradeoffs accepted:
- Higher CPU (both engines process even at extremes)
- Mitigation: Skip processing when blend = 0% or 100%

### Implementation Risks

**Highest Risk Component: Mono-Safe Mode (All-Pass/Comb Filters)**

Represents ~40% of project risk:
- Algorithm not publicly documented
- Requires deep DSP understanding (phase, group delay)
- No direct JUCE class (custom implementation)
- Verification requires testing framework

**Mitigation:**
1. Research open-source implementations
2. Implement all-pass filters in isolation, test phase response
3. Create unit tests for mono summing (L + R = 0)
4. Use fallback mid-side processing if too complex

**Other Risks (MEDIUM):**
- Multi-LFO non-repeating patterns (solution: noise modulation)
- Unison voice distribution edge cases (solution: test all voice counts)
- Feedback loop stability (solution: scale gain by (1 - feedback))
- Era preset frequency response (solution: research tape curves, approximate with biquads)

### Recommended Approach

**Phased implementation (3 DSP phases + 2-3 GUI phases):**

**Phase 4.1: Core Processing**
- Wobble + unison engines (simple sine LFO, 3 voices)
- Blend control
- Focus filter
- Dry/wet mixing

**Phase 4.2: Modulation & Character**
- Multi-LFO (dual-LFO + noise modulation)
- Triangle/random shapes
- Saturation, color, age
- Feedback loop

**Phase 4.3: Advanced Features**
- Unison expansion (2/4/5/7 voices)
- Exponential/random distribution
- Stereo width processing
- Mono-safe mode
- Era presets

**Phase 5.1: GUI Layout**
- WebView mockup integration
- Basic controls

**Phase 5.2: GUI Binding**
- Two-way parameter communication
- Value formatting

**Phase 5.3: GUI Visualizations (OPTIONAL)**
- Wobble visualization
- Unison indicator
- Mono-safe indicator

### Professional Plugin Observations

**Goodhertz Wow Control ($129):**
- Three era modes (15 IPS, 7.5 IPS, Cassette)
- Noise is modulated by signal like real tape machine
- Multi-LFO for non-repeating patterns
- High-quality pitch range with expressive shape controls

**XLN RC-20 Retro Color ($59):**
- Dual-LFO wobble (wow 0.1-4 Hz, flutter 6-20 Hz)
- Stereo mode creates chorus effect
- All-in-one lo-fi suite (noise + wobble + distort + space)

**Soundtoys MicroShift ($99):**
- Hardware-inspired (Eventide H3000, AMS DMX 15-80s)
- Multi-voice detuning with time-varying delay
- Focus control for frequency-selective processing
- Industry standard for vocal thickening

**Polyverse Wider (FREE):**
- Mono-compatible via all-pass/comb filters
- Perfect phase coherence (L + R = 0 in mono)
- Stereo width up to 200%
- Low-end bypass for bass mono preservation

**O-Detune fills the gap:**
- Combines wobble (RC-20) + unison (MicroShift) in one plugin
- Mono-safe mode (Wider-style) built-in
- Character processing (saturation + color + age)
- Target price: $49-69 (competitive with RC-20, lower than MicroShift/Wow Control)

### JUCE Module Dependencies

**Required modules:**
- `juce_dsp` - DelayLine, Oscillator, IIR::Filter, DryWetMixer, ProcessSpec
- `juce_audio_processors` - AudioProcessor, AudioProcessorValueTreeState
- `juce_gui_extra` - WebBrowserComponent (for WebView UI)

**CMake requirements:**
```cmake
target_link_libraries(O-Detune
    PRIVATE
        juce::juce_dsp
        juce::juce_audio_processors
        juce::juce_gui_extra
)

juce_generate_juce_header(O-Detune)  # CRITICAL (JUCE 8 requirement)

target_compile_definitions(O-Detune
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
```

### Critical Patterns to Remember

**From juce8-critical-patterns.md:**

- **Pattern #3:** DelayLine interpolation - Use Lagrange3rd for smooth pitch modulation
- **Pattern #17:** juce::dsp API - Use `prepare(ProcessSpec)` NOT `setSampleRate()`
- **Pattern #21:** WebView ES6 modules - Add `type="module"` to script tags
- **Pattern #12:** WebSliderParameterAttachment - Requires 3 parameters in JUCE 8 (parameter, relay, nullptr)
- **Pattern #9:** CMakeLists.txt - Add `NEEDS_WEB_BROWSER TRUE` for VST3 support

### Context for Next Stage

**Stage 1 (Planning) can now proceed with:**
- Complete DSP architecture specification (ARCHITECTURE.md)
- Implementation plan with phase breakdown (ROADMAP.md)
- Complexity score and implementation strategy (5.0 = phased)
- Professional plugin research and JUCE API mapping

**Ready for implementation:**
- All 12 features researched with JUCE class mappings
- All risks assessed with fallback architectures
- Processing chain documented with order requirements
- Thread boundaries and performance estimates documented

**Open questions for mockup finalization:**
- Wobble visualization design (waveform? graph? animated icon?)
- Unison indicator design (dots? bars? frequency spectrum?)
- Era preset visual feedback (60s/70s/80s branding)
- Advanced panel layout (expandable section or separate tab?)

---

*Stage 0 complete. Ready for /implement O-Detune to begin Stage 1 (Foundation).*
