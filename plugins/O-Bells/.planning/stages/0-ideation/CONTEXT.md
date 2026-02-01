# O-Bells - Stage 0 Research & Planning Context

**Date:** 2026-02-01
**Agent:** research-planning-agent
**Stage:** 0 (Ideation - Research & Planning complete)
**Status:** Complete

---

## Phase Summary

Stage 0 research and planning for O-Bells physical modeling bells synthesizer has been completed successfully. The plugin is classified as **maximum complexity (5.0)** requiring phased implementation across both DSP and GUI stages.

---

## Key Findings

### Complexity Assessment

**Complexity Score: 5.0 (Maximum)**

- **Parameter count:** 21 parameters → 2.0 points (capped)
- **Algorithm count:** 8 DSP components → 8 points
- **Feature complexity:** Synth + modulation → 2 points
- **Total:** 12.0 → capped at 5.0

This places O-Bells at Tier 6 complexity (synthesizer with extensive modal synthesis, ensemble voicing, and 8-voice polyphony).

### Algorithm Selection: Modal Synthesis

After thorough research, **modal synthesis** (additive synthesis of inharmonic partials) was chosen over alternatives:

**Why Modal (not Waveguide, not Sampling):**
- Bells are struck idiophones (not plucked strings) → modal physics are natural
- Waveguides better suited for strings (O-Lyrica uses waveguides for harp strings)
- Sampling lacks parameter flexibility (cannot morph materials, adjust inharmonicity in real-time)
- Modal allows independent control of each partial (frequency, amplitude, decay)

**Industry Validation:**
- Pianoteq Tubular Bells uses physical modeling (confirmed modal approach via research)
- Ableton Collision uses modal synthesis for mallet percussion
- Academic papers (IEEE, Nature/Heritage Science) confirm modal synthesis for bells

### Bell Partial Ratios Research

Extensive research into bell acoustics revealed characteristic inharmonic partial ratios:

**Church Bell (Traditional Western):**
- Fundamental: 1.0×
- Minor Third: 2.4× (defining characteristic)
- Perfect Fifth: 3.0×
- Ratios: [0.5, 1.0, 2.4, 3.0, 4.0, 5.2, 6.0, 8.0]

**Tubular Bell (More Harmonic):**
- Ratios: [1.0, 2.76, 5.40, 8.93]
- "Twangy" character from characteristic inharmonicity

**Gamelan (High Inharmonicity):**
- Ratios: [1.0, 2.1, 3.5, 5.8]
- Very dissonant, non-Western tuning

**Implementation:** INHARMONICITY parameter (0-100%) interpolates between pure harmonic → church bell → gamelan ratios.

### Ensemble Voicing Architecture

Research into professional synths (Omnisphere, Vital, Massive X) revealed standard unison detune practices:

**Unison Spread Distribution:**
- Symmetric around fundamental (one voice always on-pitch if odd count)
- Even counts (2, 4): No center voice, symmetric spread
- Odd counts (1, 3): Center voice at 0 cents, symmetric spread
- Detune range: 0-50 cents typical (O-Bells uses this range)

**Octave Layering:**
- Sub-octave (-12 semitones): Adds depth/weight
- Fundamental (0): Core bell sound
- Upper-octave (+12 semitones): Adds brilliance/shimmer
- Blend controls allow morphing from pure to layered

**CPU Impact Awareness:**
- 8 voices × 4 unison × 3 octave layers = 96 effective voices (768 oscillators worst case)
- Mitigation: Quality settings (partial count), disable unused layers, SIMD optimization

### Professional Plugin References

**1. Pianoteq Tubular Bells:**
- Physical modeling with <5% CPU (efficiency target)
- Parameters: Strike position, mallet hardness, decay time, octave stretch
- Takeaway: Modal synthesis is viable for real-time performance

**2. Spectrasonics Omnisphere - Bell Patches:**
- Hybrid: Samples + synthesis + effects
- Ensemble: Unison detune, stereo spread, octave layering
- Takeaway: Ensemble voicing is key to lush bell textures

**3. Ableton Collision:**
- Modal synthesis for mallets (bells, marimbas, vibes)
- Uses 10-20 partials for high quality, minimal CPU
- Takeaway: 8 partials is conservative, can increase if needed

### JUCE Module Requirements

**Core Modules:**
- `juce_audio_basics` - Synthesiser, SynthesiserVoice, ADSR, MidiBuffer
- `juce_dsp` - Oscillator (SIMD-optimized), WaveShaper, IIR filters
- `juce_audio_processors` - AudioProcessorValueTreeState (APVTS)
- `juce_gui_extra` - WebBrowserComponent (for WebView UI)

**Critical JUCE 8 Patterns:**
- IS_SYNTH TRUE flag required (instruments, not effects)
- BusesProperties: Output-only (no input bus for synths)
- WebView: NEEDS_WEB_BROWSER TRUE in CMakeLists.txt
- ES6 module loading for parameter binding (type="module")
- WebSliderParameterAttachment: 3-parameter constructor (parameter, relay, nullptr)

### Implementation Risks Identified

**1. CPU Management (MEDIUM-HIGH Risk)**
- Worst case: 768 oscillators (8 voices × 4 unison × 8 partials × 3 octaves)
- Mitigation: Quality settings, SIMD, disable unused layers, profiling
- Target: <60% CPU single core @ 48kHz, 256 samples

**2. Voice Stealing Artifacts (LOW Risk)**
- Long bell decays make stealing audible
- Mitigation: 5ms fade-out, prioritize release-phase voices

**3. Sympathetic Resonance Complexity (MEDIUM Risk)**
- Cross-voice coupling can cause feedback loops
- Mitigation: Limit coupling strength (15% max), make optional (disabled by default)

**4. Ensemble CPU Multiplication (MEDIUM Risk)**
- User may set all parameters to max without understanding CPU cost
- Mitigation: Document CPU tradeoffs, default to low unison (1-2), quality presets

---

## Architectural Decisions

### Decision 1: 8-Voice Polyphony (Not 16, Not 4)

**Chosen:** 8 simultaneous voices with oldest-first voice stealing

**Rationale:**
- Ensemble voicing multiplies effective voices (4 unison = 32 bells)
- Most bell music uses <8 simultaneous notes
- CPU budget: 8 × 4 unison × 8 partials = 256 oscillators (reasonable)
- Reference: O-Lyrica targets 16-32 but uses cheaper waveguide synthesis

**Tradeoff:** Voice stealing audible with long decays (mitigated by fade-out)

### Decision 2: Church Bell Partials as Default (50% Inharmonicity)

**Chosen:** INHARMONICITY defaults to 50% (church bell ratios)

**Rationale:**
- Church bells most recognizable (minor third character)
- Balanced starting point (not too harmonic, not too dissonant)
- BRIEF mentions "characteristic bell partials (minor third at ~2.4× fundamental)"

**Tradeoff:** Less "pure" than tubular bells (users can reduce to 20-30% for orchestral use)

### Decision 3: Discrete Unison Count 1-4 (Not Continuous)

**Chosen:** Integer parameter (1, 2, 3, 4 bells per voice)

**Rationale:**
- Discrete steps easier to understand (1.7 bells makes no sense)
- Allows simple presets: Solo (1), Duo (2), Trio (3), Quartet (4)
- Reference: Omnisphere, Vital use discrete unison counts

**Tradeoff:** Cannot smoothly morph from 1 to 4 (stepped increase, but can fade)

---

## Research Sources

### Academic Papers

1. **"Tubular Bells: A Physical and Algorithmic Model" (IEEE, Rabenstein & Koch)**
   - Confirms modal synthesis is standard approach for bells
   - Partial ratios: 1.0, 2.76, 5.40, 8.93 (tubular bell)

2. **"Physical modelling techniques for the dynamical characterization and sound synthesis of historical bells" (Nature/Heritage Science)**
   - Multidisciplinary approach: 3D morphology, FEA, modal extraction
   - Validates simplification for synthesis (8 partials is acceptable)

3. **"Partial Frequencies and Chladni's Law in Church Bells" (SCIRP)**
   - Church bell ratios: 1:2:2.4:3:4 (minor third at 2.4×)
   - Inharmonicity is defining characteristic of bell timbre

### Industry Resources

- Sound on Sound: "Synthesizing Bells" tutorial
- JUCE Forum: "Efficient modal synthesis?" discussion (community consensus: 10-20 partials reasonable)
- Pianoteq user forums: Tubular Bells add-on discussions

### Professional Plugins Researched

- Pianoteq Tubular Bells (physical modeling, <5% CPU)
- Spectrasonics Omnisphere (bell patches, ensemble voicing)
- Ableton Collision (modal synthesis, 10-20 partials)

---

## Constraints and Considerations

### CPU Budget

**Target:** <60% CPU on single core (worst case: 8 voices, 4 unison, 3 octave layers)

**Strategies:**
- Use `juce::dsp::Oscillator` with SIMD optimization
- Disable inactive octave layers (OCTAVE_BLEND = 0%)
- Early-exit for silent voices (amplitude < -60dB)
- Quality settings: Low (4 partials), Med (6 partials), High (8 partials)

### Thread Safety

**No custom locks needed:**
- APVTS handles parameter thread safety (atomic reads/writes)
- `juce::Synthesiser` handles voice allocation thread safety
- Each voice has independent state (no shared mutable data)

### Aesthetic Integration

**Ouaricon Botanical Theme:**
- Snail motif (spiral shell echoes bell geometry and harmonic series)
- Color palette: Warm amber, bronze, cream, aged gold (bell metal patina)
- Two-panel design: Main (7 params + ensemble) / Advanced (10+ params)
- Hero image: Architectonica perspectiva snail species

**Asset location:** `/Users/taylorbrook/Dev/Ouaricon Audio Images/insects/snails_spciesgnra12kiene_0169.png`

---

## Open Questions for Stage 1 Planning

1. **Parameter specification format:**
   - Extract 21 parameters from BRIEF.md into formal parameter-spec.md
   - Define: ID, type, range, default, units, description

2. **Mockup creation timing:**
   - Create mockup during Stage 1 Planning (recommended for early visualization)
   - OR defer to GUI stage (faster to implementation)

3. **Quality settings implementation:**
   - Build-time constant (partial count set at compile)
   - Runtime parameter (user selects Low/Med/High quality)
   - **Recommendation:** Runtime parameter (more flexible for CPU management)

4. **Sympathetic resonance inclusion:**
   - Implement in Phase 3.3 (advanced features) as optional
   - OR defer to future version (reduce initial scope)
   - **Recommendation:** Implement but default to 0% (disabled, no CPU cost)

---

## Next Steps

**Stage 1 Planning (Next):**
1. Create `parameter-spec.md` - Extract all 21 parameters from BRIEF
2. Review ARCHITECTURE.md and ROADMAP.md for approval
3. Optional: Create UI mockup (v1-ui.yaml) for visualization
4. Proceed to Stage 1 Foundation after approval

**Stage 1 Foundation (After Planning):**
1. Create CMakeLists.txt with IS_SYNTH TRUE, NEEDS_WEB_BROWSER TRUE
2. Create PluginProcessor stub with output-only BusesProperties
3. Set up APVTS with all 21 parameters
4. Verify build (ninja O-Bells_VST3 O-Bells_AU)

**Stage 3 DSP (Phased Implementation):**
- Phase 3.1: Core modal synthesis (8 partials, single voice)
- Phase 3.2: Polyphony (8 voices) + strike dynamics
- Phase 3.3: Ensemble voicing + advanced features

**Stage 4 GUI (Phased Implementation):**
- Phase 4.1: Main panel layout (7 knobs + ensemble)
- Phase 4.2: Parameter binding (APVTS ↔ WebView)
- Phase 4.3: Advanced panel + polish

---

## Success Criteria Met

- ✅ ARCHITECTURE.md created with all required sections (11 sections complete)
- ✅ ROADMAP.md created with complexity assessment and phase breakdown
- ✅ Every feature researched and documented
- ✅ JUCE classes identified with module dependencies
- ✅ High-risk features have fallback architectures
- ✅ Integration analysis covers dependencies, interactions, processing order, threads
- ✅ Professional plugins researched (Pianoteq, Omnisphere, Collision)
- ✅ Academic validation (IEEE, Nature, SCIRP papers)

**Ready for Stage 1 Planning:** All Stage 0 research deliverables complete.

---

**Prepared by:** research-planning-agent
**Date:** 2026-02-01
**Stage 0 Status:** ✅ Complete
