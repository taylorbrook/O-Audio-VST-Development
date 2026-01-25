# Project Research Summary

**Project:** O-Bass
**Domain:** Audio Plugin - Psychoacoustic Bass Enhancement
**Researched:** 2026-01-22
**Confidence:** HIGH

## Executive Summary

O-Bass is a psychoacoustic bass enhancement plugin that generates harmonics to create perceived bass weight on playback systems that cannot reproduce low frequencies. This technique works by exploiting the "missing fundamental" phenomenon where the brain reconstructs bass perception from harmonic content in the 100-400Hz range. The implementation requires parallel processing: extract bass via crossover, generate harmonics through nonlinear processing, shape and blend back with the original signal.

The recommended approach uses two distinct modes to differentiate O-Bass in a crowded market: Clean mode employs psychoacoustic harmonic synthesis (full-wave integration from verified MATLAB algorithms) for transparent enhancement, while Colored mode applies analog-modeled saturation for musical warmth. The core stack leverages existing JUCE DSP classes (LinkwitzRileyFilter for crossover, WaveShaper for saturation, IIR filters for harmonic shaping) and integrates with the established Ouaricon plugin architecture (WebView UI, AudioProcessorValueTreeState parameters, OuariconPresetManager).

The primary risks are aliasing artifacts from nonlinear processing (mitigated with oversampling), phase cancellation on mono playback (solved by processing bass in mono internally), and over-processing leading to boomy/unnatural sound (addressed through auto-limiting and diminishing returns intensity curves). Secondary concerns include maintaining low latency for mix bus usage, preserving transients during enhancement, and ensuring the effect translates across playback systems. The minimal UI constraint (3-5 controls) actually reduces risk by forcing algorithmic intelligence over parameter complexity.

## Key Findings

### Recommended Stack

The JUCE DSP module provides all required building blocks for psychoacoustic bass enhancement. The signal flow uses crossover filtering to isolate bass content, applies harmonic generation through nonlinear processing (full-wave integration for Clean mode, waveshaping for Colored mode), shapes the result with bandpass filtering, and mixes back with the original high-frequency path.

**Core technologies:**
- **JUCE dsp::LinkwitzRileyFilter**: Crossover splitting at 60-80Hz — purpose-built for flat-sum response with 4th-order rolloff and TPT structure for stability
- **JUCE dsp::WaveShaper**: Harmonic generation via custom transfer functions — memoryless nonlinearity with low latency for saturation in Colored mode
- **JUCE dsp::IIR::Filter**: Bandpass filtering and harmonic shaping — flexible coefficient design with low CPU overhead for controlling harmonic range (60-400Hz)
- **JUCE dsp::Oversampling**: Anti-aliasing for Clean mode — 2x-4x oversampling prevents waveshaper aliasing, critical for transparent processing
- **Full-Wave Integrator**: Psychoacoustic harmonic synthesis — verified MATLAB algorithm that generates harmonics at integer multiples through recursive integration with zero-crossing reset

**Critical implementation detail:** The full-wave integrator algorithm (`y[n] = (u[n] > 0 && u[n-1] <= 0) ? 0 : y[n-1] + u[n-1]`) is the core of Clean mode. It produces a richer harmonic series than simple waveshaping and aligns with peer-reviewed psychoacoustic research on the missing fundamental phenomenon.

### Expected Features

Competitive analysis shows successful minimal bass enhancers (Waves R-Bass, Fire Boy) operate with 3-4 core controls, while feature-rich alternatives (bx_subsynth, MaxxBass) serve users who want granular control. O-Bass targets the minimal segment with Clean/Colored mode switching as its key differentiator.

**Must have (table stakes):**
- Target frequency control (40-200Hz) — users must tune enhancement to source material; kick drums at ~50Hz, bass guitar at ~80Hz, synth bass varies
- Intensity/amount control (0-100%) — core function determining how much harmonic generation to apply
- Output gain (+/- 12-18dB) — enhancement adds energy; users need level matching for A/B comparison
- Harmonic generation algorithm — without this it's just EQ; the psychoacoustic mechanism is the product
- Clean audio path — no unwanted clicks, distortion, or artifacts when effect is subtle
- Small speaker translation — primary use case; harmonics must be in reproducible range (100-400Hz)

**Should have (competitive differentiators):**
- Clean/Colored mode switch — versatility without parameter bloat; core differentiator per PROJECT.md goal
- Phase-coherent processing — Denise Bass XXL markets "zero phasing" as key feature; quality differentiator
- Oversampling — expected quality standard for transparent harmonic generation
- Even/odd harmonic blend — control over character (even = warm/tube, odd = aggressive) could be single knob in Colored mode

**Defer (anti-features for v1):**
- Multi-band crossover controls — adds 3+ knobs, violates minimal philosophy
- Built-in compressor section — scope creep, users have dedicated compressors, violates plugin focus
- Spectrum analyzer — encourages visual mixing over listening, adds UI complexity
- Subharmonic synthesis — different technique entirely, requires different algorithm and phase complexity
- Mid/side processing — per PROJECT.md out of scope
- Multiple saturation modes — feature creep; clean/colored covers the need

**MVP recommendation:** 4 controls total: Frequency knob (40-200Hz), Enhance knob (intensity), Output knob (gain), Mode toggle (Clean/Colored). This matches R-Bass simplicity while adding mode differentiation.

### Architecture Approach

O-Bass integrates with the existing Ouaricon plugin architecture using the established PluginProcessor/PluginEditor pattern with WebView UI. The DSP pipeline uses parallel processing where the original signal is split via crossover, the low-frequency content is processed through either psychoacoustic enhancement (Clean mode) or analog saturation (Colored mode), then recombined with the high-frequency path.

**Major components:**
1. **OBassAudioProcessor** — Core DSP processing, parameter management via AudioProcessorValueTreeState, atomic metering for UI updates, integrates OuariconPresetManager for preset system following existing suite patterns
2. **CrossoverFilter utility** — Linkwitz-Riley 2nd order crossover that splits signal into bass (below ~80Hz) and rest; ensures flat-sum response when recombined
3. **HarmonicSynthesizer (Clean mode)** — Full-wave integrator implementation generating psychoacoustic harmonics, followed by bandpass filtering (60-250Hz) to shape harmonic content
4. **BassSaturator (Colored mode)** — Simplified from OuariconSaturationModeling using asymmetric soft clipping with WaveShaper, produces both even and odd harmonics for warmth
5. **OBassAudioProcessorEditor** — WebView UI hosting with WebSliderRelay pattern for parameter binding, timer-based metering updates at 30Hz, follows established Ouaricon visual language

**Signal flow:** Input splits via crossover → Low path processes through selected mode (Clean = integrator + bandpass, Colored = saturation + warmth filter) → Enhancement gain → Mix with high path → Output gain → Output

**Key architectural decision:** Process bass in mono internally regardless of stereo input. This prevents phase cancellation when summed to mono playback (critical pitfall from research). The enhancement is applied to summed bass, then redistributed to stereo.

### Critical Pitfalls

Research identified 11 pitfalls across Critical, Moderate, and Minor categories. The top 5 that must be addressed in core architecture:

1. **Aliasing from Nonlinear Processing** — Harmonic generation creates frequencies above Nyquist that fold back as inharmonic noise. Bass note at 50Hz generates harmonics at 100Hz, 150Hz, 200Hz... higher orders quickly exceed 22.05kHz and mirror back. Prevention: Implement 2x-4x oversampling for Clean mode, use ADAA (Antiderivative Anti-Aliasing) where possible, test with sine sweeps to identify aliasing. This must be built into architecture from the start.

2. **Phase Cancellation on Sum-to-Mono** — If harmonic generation differs between L/R channels, enhancement vanishes or becomes hollow on mono playback (phone speakers, club systems). Prevention: Process bass in mono internally (sum L+R below crossover), apply enhancement to mono signal, redistribute to stereo. Critical for professional use in EDM/club contexts.

3. **Over-Processing Leading to Boomy/Unnatural Sound** — Psychoacoustic enhancement is powerful; a little goes a long way. Without careful gain staging, generated harmonics easily exceed original signal level. Prevention: Implement auto-limiting or soft ceiling on enhancement amount, design intensity curve with diminishing returns (not linear), add visual feedback showing "safe zone", consider automatic gain compensation.

4. **Latency Breaking Mix Bus Usage** — Oversampling filters, linear-phase crossovers, and lookahead processing all add latency. Mix buses typically limited to 8192 samples latency compensation. Prevention: Target <5ms total latency (220 samples at 44.1kHz), use minimum-phase Linkwitz-Riley filters, avoid lookahead, report accurate latency to host for PDC, consider zero-latency mode.

5. **Transient Smearing in Enhancement** — Full-wave integration and oversampling filters can round bass transients, losing "punch." Research notes: "varying weights between transient and steady-state components may cause unnatural switching effects." Prevention: Implement transient detection and preserve/bypass transients, use minimum-phase filters, blend enhancement with original preserving transient envelope, test with percussive content (slap bass, punchy kicks).

**Additional moderate pitfall:** Intermodulation distortion on polyphonic material — nonlinear processing on full mixes creates sum/difference frequencies that aren't harmonically related to original content. A bass note at 80Hz and kick at 60Hz create IMD products at 140Hz and 20Hz. Mitigate by applying saturation/harmonics to isolated frequency bands (crossover helps) and testing explicitly with polyphonic material.

## Implications for Roadmap

Based on research, the natural phase structure follows dependency order: foundation (DSP architecture), core algorithm implementation, mode differentiation, and finally UI/polish. The critical pitfalls inform what must be addressed in each phase.

### Phase 1: Core DSP Foundation
**Rationale:** The crossover, mono bass processing, and parameter framework are foundational. All subsequent work depends on this architecture. This phase addresses the most critical pitfalls (aliasing, phase cancellation, latency) that cannot be retrofitted later.

**Delivers:**
- Functional PluginProcessor with parameter layout (AMOUNT, FREQUENCY, OUTPUT, MODE)
- Crossover filter implementation (Linkwitz-Riley at configurable frequency)
- Mono bass processing path
- Basic gain staging and output
- Oversampling infrastructure for Clean mode
- Latency reporting to host

**Addresses (from FEATURES.md):**
- Clean audio path (foundational architecture)
- Phase-coherent processing (mono bass routing)

**Avoids (from PITFALLS.md):**
- Phase cancellation on sum-to-mono (mono processing path built in)
- Latency breaking mix bus usage (minimum-phase filters, <5ms target)
- Allocations in processBlock (pre-allocated buffers in prepareToPlay)

**Stack elements:**
- JUCE dsp::ProcessorDuplicator for stereo IIR filters
- JUCE dsp::ProcessSpec for DSP initialization
- AudioProcessorValueTreeState for parameters
- JUCE dsp::Oversampling class setup

**Research flag:** Standard patterns (skip research-phase). JUCE DSP is well-documented, crossover design is established.

### Phase 2: Psychoacoustic Algorithm (Clean Mode)
**Rationale:** Clean mode is the core differentiator and most novel component. The full-wave integrator algorithm from MATLAB research must be implemented correctly before layering on Colored mode. This validates the fundamental value proposition of O-Bass.

**Delivers:**
- Full-wave integrator implementation (verified algorithm)
- Bandpass filtering for harmonic shaping (60-250Hz)
- Harmonic amplitude weighting (research-verified decay ratios)
- Enhancement intensity control
- Oversampling processing path

**Addresses (from FEATURES.md):**
- Harmonic generation algorithm (psychoacoustic mechanism)
- Small speaker translation (harmonics in 100-400Hz range)
- Target frequency control (crossover tuning)

**Avoids (from PITFALLS.md):**
- Aliasing from nonlinear processing (oversampling active)
- Translation failure across playback systems (harmonics in reproducible range)

**Stack elements:**
- Full-wave integrator state machine
- JUCE dsp::IIR::Filter for bandpass
- JUCE dsp::Oversampling 2x-4x

**Research flag:** May need research-phase if full-wave integrator proves difficult. The algorithm is well-specified but implementation details (DC blocking, stability) may require iteration.

### Phase 3: Colored Mode (Saturation)
**Rationale:** With Clean mode validated, Colored mode adds the character option. This leverages existing knowledge from OuariconSaturationModeling but simplified to a single tube-like saturation curve optimized for bass.

**Delivers:**
- Asymmetric waveshaping function (even + odd harmonics)
- Drive parameter mapping to AMOUNT control
- Warmth filter for subtle LF boost
- Mode switching logic

**Addresses (from FEATURES.md):**
- Clean/Colored mode switch (key differentiator)
- Even/odd harmonic blend (baked into colored algorithm)

**Avoids (from PITFALLS.md):**
- Over-processing (Colored mode has gentler curve than aggressive saturation)
- Intermodulation distortion (tested with polyphonic material)

**Stack elements:**
- JUCE dsp::WaveShaper with custom transfer function
- Reference to OuariconSaturationModeling for saturation patterns

**Research flag:** Standard patterns. Can reference existing OuariconSaturationModeling codebase and simplify.

### Phase 4: Algorithm Refinement
**Rationale:** With both modes functional, this phase addresses the moderate pitfalls through iterative testing and refinement. These issues cannot be fully validated without real-world material testing.

**Delivers:**
- Transient preservation mechanism
- Intensity curve refinement (diminishing returns)
- Auto-limiting on enhancement amount
- Testing across multiple playback systems
- IMD testing with polyphonic material

**Addresses (from FEATURES.md):**
- Table stakes quality expectations (no artifacts, clean when subtle)

**Avoids (from PITFALLS.md):**
- Transient smearing (preserve/bypass transients)
- Over-processing leading to boomy sound (auto-limiting, visual safe zones)
- Intermodulation distortion (validation with full mixes)

**Research flag:** May need targeted research on transient detection algorithms if smearing becomes audible. Most can be addressed through testing.

### Phase 5: WebView UI
**Rationale:** With DSP complete and stable, UI development can proceed without blocking algorithm work. The UI follows established Ouaricon patterns so minimal exploration needed.

**Delivers:**
- WebView UI with 4-control layout (Frequency, Enhance, Output, Mode)
- Parameter relay setup (WebSliderRelay, WebToggleButtonRelay)
- Metering visualization (input/output levels, enhancement activity)
- Ouaricon visual language (paper texture, botanical artwork)

**Addresses (from FEATURES.md):**
- Output gain (visual knob)
- Intensity/amount control (visual knob)
- Mode switch (toggle button)

**Avoids (from PITFALLS.md):**
- Inadequate monitoring feedback (visual metering shows what plugin does)

**Stack elements:**
- JUCE WebBrowserComponent
- WebSliderRelay/WebSliderParameterAttachment pattern
- Timer-based metering updates (30Hz)

**Research flag:** Standard patterns. Copy from existing OuariconSaturationModeling/OuariconComp UI.

### Phase 6: Polish & Presets
**Rationale:** Final phase handles quality-of-life features and optimization. Preset system follows established Ouaricon patterns.

**Delivers:**
- OuariconPresetManager integration
- Factory presets (Kick Enhancement, Bass Guitar, Mix Bus Glue, etc.)
- CPU optimization pass
- Cross-format testing (VST3, AU, Standalone)
- Documentation

**Addresses (from FEATURES.md):**
- Complete product experience

**Avoids (from PITFALLS.md):**
- CPU spikes on dense material (profiling with worst-case content)

**Research flag:** Standard patterns. OuariconPresetManager is header-only, minimal adaptation.

### Phase Ordering Rationale

**Why this order:**
1. **Foundation first** — Phase 1 establishes architecture that cannot be changed later without major refactoring. The mono bass processing, crossover design, and oversampling infrastructure must be correct from the start.
2. **Core value proposition** — Phase 2 validates O-Bass's reason to exist (psychoacoustic enhancement). If this doesn't work, the product has no foundation.
3. **Differentiation layer** — Phase 3 adds Colored mode as competitive differentiator only after Clean mode is proven.
4. **Testing-driven refinement** — Phase 4 addresses pitfalls that require real-world material testing and cannot be designed in isolation.
5. **UI last** — Phase 5 can proceed independently once DSP is stable. Following this order prevents UI churn from algorithm changes.
6. **Polish completes** — Phase 6 handles preset system and optimization that assume stable DSP and UI.

**Why this grouping:**
- Phases 1-3 are DSP-focused and could have overlapping work if developer is comfortable with parallel streams
- Phase 4 is validation/testing which requires Phases 2-3 complete
- Phases 5-6 are polish and can happen in parallel with Phase 4 refinement

**How this avoids pitfalls:**
- Critical pitfalls (aliasing, phase cancellation, latency) addressed in Phase 1 foundation
- Moderate pitfalls (transient smearing, over-processing, IMD) addressed in Phase 4 with real testing
- Minor pitfalls (CPU spikes, inadequate feedback) addressed in Phases 5-6 polish

### Research Flags

**Phases likely needing deeper research:**
- **Phase 2 (Psychoacoustic Algorithm):** Full-wave integrator is well-specified in MATLAB docs but implementation details (DC blocking, numerical stability, denormal handling) may require iteration. If issues arise, run `/gsd:research-phase` focused on "full-wave integration stability and DC offset handling."

**Phases with standard patterns (skip research-phase):**
- **Phase 1 (Core DSP Foundation):** JUCE DSP crossover filters and AudioProcessorValueTreeState patterns are well-documented with examples in existing codebase
- **Phase 3 (Colored Mode):** Can directly reference OuariconSaturationModeling implementation
- **Phase 5 (WebView UI):** Follow existing OuariconSaturationModeling/OuariconComp UI patterns exactly
- **Phase 6 (Polish):** OuariconPresetManager is header-only with established usage

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All components verified via official JUCE documentation; full-wave integrator algorithm from peer-reviewed MATLAB reference |
| Features | HIGH | Competitive analysis of 6+ commercial products shows clear patterns; MVP recommendation (4 controls) validated against successful minimal plugins like Waves R-Bass |
| Architecture | HIGH | Based on existing Ouaricon codebase analysis; DSP patterns verified in JUCE tutorials; parallel processing approach is standard for enhancement plugins |
| Pitfalls | MEDIUM-HIGH | Critical pitfalls verified via multiple sources (Science of Sound, ResearchGate papers, community consensus); moderate pitfalls identified through product reviews and forum discussions |

**Overall confidence:** HIGH

The research converges on clear recommendations across all dimensions. The psychoacoustic bass enhancement domain is well-understood with established techniques (missing fundamental, harmonic weighting ratios from IEEE research). JUCE provides all required DSP primitives. The main uncertainty is in algorithm refinement (Phase 4) which requires iterative testing rather than upfront research.

### Gaps to Address

**Algorithm parameter tuning:** Research provides theoretical ratios for harmonic amplitude weighting (-6dB per harmonic from Larsen et al. IEEE), but real-world optimal values may differ. Plan for A/B testing in Phase 4 with ability to adjust weighting curves.

**Transient preservation specifics:** Research identifies transient smearing as a pitfall but doesn't specify exact detection algorithm. During Phase 4, experiment with envelope follower vs. RMS difference vs. spectral flux methods. This is implementation detail rather than architectural concern.

**Crossover frequency sweet spot:** Research suggests 60-80Hz range, but optimal default may be content-dependent. Phase 4 should test with variety of source material (kick drums at 50Hz, bass guitar at 80Hz, 808s at 55Hz) to determine if single default works or if presets should target different ranges.

**Clean vs Colored character balance:** The line between "transparent enhancement" (Clean) and "musical warmth" (Colored) is subjective. Phase 4 refinement should validate with user testing that the two modes feel distinct enough to justify the mode switch.

**Mix bus suitability validation:** While architecture targets <5ms latency and mono bass processing for phase coherence, real-world mix bus testing in Phase 4 will validate whether O-Bass truly works on master without issues. Test specifically with dense arrangements and busses with other processing.

All gaps are addressable through iterative testing in Phase 4 rather than requiring additional upfront research. The architecture from Phases 1-3 provides the flexibility to adjust these parameters without refactoring.

## Sources

### Primary (HIGH confidence)
- [JUCE dsp::LinkwitzRileyFilter Documentation](https://docs.juce.com/master/classdsp_1_1LinkwitzRileyFilter.html) — Crossover filter design, TPT structure, flat-sum response characteristics
- [JUCE dsp::WaveShaper Documentation](https://docs.juce.com/master/structdsp_1_1WaveShaper.html) — Memoryless nonlinearity implementation, transfer function patterns
- [JUCE dsp::Oversampling Documentation](https://docs.juce.com/master/classdsp_1_1Oversampling.html) — Oversampling factors, filter options, latency characteristics
- [MathWorks: Psychoacoustic Bass Enhancement for Band-Limited Signals](https://www.mathworks.com/help/audio/ug/psychoacoustic-bass-enhancement-for-band-limited-signals.html) — Full-wave integrator algorithm specification, verified reference implementation
- [IEEE: A psychoacoustic bass enhancement system with improved transient and steady-state performance](https://ieeexplore.ieee.org/document/6287837/) — Harmonic weighting ratios, transient preservation research
- [Wikipedia: Missing fundamental](https://en.wikipedia.org/wiki/Missing_fundamental) — Foundational psychoacoustics, residue pitch phenomenon
- Existing Ouaricon codebase (OuariconSaturationModeling, OuariconAnalogEQ, OuariconComp) — Architecture patterns, WebView UI integration, parameter management

### Secondary (MEDIUM confidence)
- [Waves: Bass Plugins and Sub Enhancers Compared](https://www.waves.com/bass-plugins-and-sub-enhancers-compared) — R-Bass vs MaxxBass feature comparison, intensity warnings
- [Denise Audio Bass XXL Product Page](https://www.deniseaudio.com/plugins/bass-xxl) — Phase-free marketing claims, competitive positioning
- [Universal Audio Precision Enhancer Hz Manual](https://help.uaudio.com/hc/en-us/articles/33536774357780-Precision-Enhancer-Hz-Manual) — Mode-based interface design, frequency ranges for different sources
- [Science of Sound: Oversampling in Distortion Effects](https://science-of-sound.net/2016/06/oversampling-distortion-effects/) — Aliasing mechanics, oversampling requirements
- [ResearchGate: Psychoacoustic Bass Enhancement with Improved Transient Performance](https://www.researchgate.net/publication/236843786_A_psychoacoustic_bass_enhancement_system_with_improved_transient_and_steady-state_performance) — Transient smearing identification, switching artifact warnings
- [Black Ghost Audio: How to Improve Mono Compatibility](https://www.blackghostaudio.com/blog/how-to-improve-mono-compatibility) — Phase cancellation testing, correlation metering
- [Sound-Au: Intermodulation Distortion](https://sound-au.com/articles/intermodulation2.htm) — IMD mechanics on polyphonic material

### Tertiary (LOW confidence, community consensus)
- [KVR Audio: Bass Enhancement Discussion](https://www.kvraudio.com/forum/viewtopic.php?t=612874) — User preferences between psychoacoustic and subharmonic approaches
- [Gearspace: Waves MaxxBass & RBass threads](https://gearspace.com/board/rap-hip-hop-engineering-and-production/386139-waves-maxxbass-amp-rbass.html) — Professional opinions on transparency vs. color
- [Pro Audio Files: 7 Favorite Bass Enhancer Plugins](https://theproaudiofiles.com/bass-enhancer-plugins/) — Use cases and professional workflows

---
*Research completed: 2026-01-22*
*Ready for roadmap: yes*
