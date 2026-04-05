# O-Formant Stage 0 Context

**Date:** 2026-04-04
**Agent:** research-planning-agent
**Duration:** Stage 0 Research & Planning

---

## Discuss Phase Findings

### Key Architectural Decisions

1. **MPESynthesiser over basic Synthesiser:** MPE is core to the expressive vocal concept (pressure -> breathiness, slide -> vowel Y). Using MPESynthesiser gives us this for free with enableLegacyMode() fallback. Proven pattern from O-Prism/O-Lyrica.

2. **Parallel formant topology (not cascade):** Simpler, gives per-formant gain control, and works identically for both vowel and consonant branches. Cascade can be added in v2 as an option.

3. **Direct LF computation with PolyBLEP (not wavetable) for v1:** Faster development iteration. Wavetable would give better anti-aliasing (~90dB vs ~60dB) but requires pre-computation infrastructure. The direct approach lets us validate the entire vocal engine first, then upgrade the source in polish.

4. **Custom biquad structs (not juce::dsp::IIR::Filter):** 80 filter instances (5 x 16 voices) makes ProcessorState overhead unacceptable. Custom 32-byte struct is cache-friendly and processes one sample in 5 multiplies. Coefficient computation still delegates to JUCE's proven ArrayCoefficients.

5. **Shepard interpolation (not barycentric/RBF):** With only 5 data points, Shepard is the best balance. The tunable power parameter doubles as the "Vowel Focus" control -- an elegant mapping. Log-domain frequency interpolation ensures perceptually smooth transitions.

6. **No built-in effects for v1:** BRIEF explicitly defers reverb/chorus to v1.1+. Keeps scope focused on the core vocal engine.

### Research Validation

- **JUCE API verified (8.0.4):** MPESynthesiser, MPESynthesiserVoice, ADSR, IIR::ArrayCoefficients::makeBandPass, Gain, Random all confirmed in local JUCE source.
- **enableLegacyMode() signature confirmed:** `enableLegacyMode(int pitchbendRange = 2, Range<int> channelRange = Range<int>(1, 17))` on MPESynthesiserBase.
- **IS_SYNTH TRUE required (juce8-critical-patterns.md #22):** Plugin must declare IS_SYNTH TRUE + NEEDS_MIDI_INPUT TRUE in CMakeLists.txt.
- **Output-only bus config (juce8-critical-patterns.md #4):** Synths use `BusesProperties().withOutput()` only -- no input bus.

### Complexity Assessment

- **Raw score: 8.2** (capped at 5.0). This is a genuinely complex plugin with novel DSP.
- **Highest risk:** LF Glottal Pulse Model (Newton-Raphson solvers, PolyBLEP, numerical stability at extreme Rd)
- **Mitigations documented:** 3 fallback levels for glottal model (pre-tabulated values, wavetable bank, simple morph)
- **Staging strategy:** Build core vocal engine first (Phase 2.1), add LF model complexity second (already in Phase 2.1), then expression/consonants (Phase 2.2), then output/polish (Phase 2.3)

### Constraints and Boundaries

- **21 parameters (not 22):** BRIEF and parameter-spec-draft both specify 21. Deep research doc mentions reverbMix as a 22nd -- we follow the spec, not the research doc.
- **Zero latency:** No oversampling, no FFT, no lookahead. All processing is causal and immediate.
- **16 voices default:** ~1.5% single CPU core at 48kHz. Well within budget.
- **32-bit float precision:** Sufficient for formant filters and glottal model. Double precision unnecessary given the per-sample operations are lightweight.

### Open Questions for Implementation

1. **Newton-Raphson iteration count:** The research docs suggest 20-30 max iterations. Need to verify convergence across full Rd range in practice. If convergence issues arise, fall back to pre-tabulated alpha/epsilon.
2. **Plosive burst feel:** 10-25ms is the research recommendation, but exact timing needs to be tuned by ear. Plan to make burst duration velocity-dependent.
3. **XY pad WebView relay:** Standard WebSliderRelay maps to a single parameter. The XY pad needs to update two parameters (vowelX, vowelY) from a single gesture. May need custom JavaScript relay or two separate relays with coordinated update.

---

## Files Created

- `plugins/O-Formant/.planning/research/ARCHITECTURE.md` -- Complete DSP architecture specification
- `plugins/O-Formant/.planning/ROADMAP.md` -- Implementation plan with phased breakdown
- `plugins/O-Formant/.planning/stages/0-ideation/CONTEXT.md` -- This file

## Files Referenced

- `plugins/O-Formant/.planning/BRIEF.md` -- Creative brief (input)
- `plugins/O-Formant/.planning/parameter-spec-draft.md` -- Parameter specification (input)
- `research/O-Formant-deep-research.md` -- Master synthesis research
- `research/O-Formant-market-research.md` -- Market validation
- `research/2d-vowel-morph-xy-pad.md` -- XY pad geometry
- `research/consonant-noise-synthesis.md` -- Consonant system
- `research/glottal-pulse-modeling-deep-dive.md` -- LF model detail
- `troubleshooting/patterns/juce8-critical-patterns.md` -- Critical JUCE 8 patterns
