# Feature Landscape: Psychoacoustic Bass Enhancement

**Domain:** Bass enhancement plugins for music production
**Researched:** 2026-01-22
**Confidence:** HIGH (verified against multiple commercial products and industry sources)

## Executive Summary

The bass enhancement plugin market divides into two approaches: psychoacoustic enhancement (generating harmonics to trick perception) and subharmonic synthesis (generating actual low frequencies). For O-Bass with its minimal UI goal, psychoacoustic enhancement is the correct approach—it works on small speakers, requires fewer controls, and aligns with the "results over tweaking" philosophy.

The competitive landscape shows a clear pattern: successful minimal plugins (Waves R-Bass, Fire Boy, Precision Enhancer Hz) succeed with 3-4 core controls, while feature-rich plugins (bx_subsynth, MaxxBass) serve different users who want granular control. O-Bass should target the minimal segment with clean/colored mode switching as its differentiator.

---

## Table Stakes

Features users expect from any bass enhancement plugin. Missing these means the product feels incomplete or broken.

| Feature | Why Expected | Complexity | Implementation Notes |
|---------|--------------|------------|---------------------|
| **Target frequency control** | Users must tune enhancement to source material; fundamentals vary widely (kick ~50Hz, bass guitar ~80Hz, synth bass ~40-100Hz) | Low | Single knob, 40-200Hz range typical |
| **Intensity/amount control** | Core function—how much enhancement to apply; every competitor has this | Low | Main processing amount, 0-100% |
| **Output gain** | Enhancement adds energy; users need level matching for A/B comparison | Low | Simple gain stage, +/- 12-18dB |
| **Harmonic generation** | Core psychoacoustic mechanism—without this, it's just EQ | Medium | Algorithm that generates upper harmonics of detected fundamental |
| **Clean audio path** | No unwanted artifacts, clicks, or distortion when effect is subtle | Medium | Proper signal flow, oversampling consideration |
| **Small speaker translation** | Primary use case for psychoacoustic enhancement | Medium | Built into algorithm design—harmonics must be in reproducible range |

**Critical insight:** Waves R-Bass succeeds with exactly 3 controls (frequency, intensity, gain). This proves minimal is viable. Users praise it as "set it and forget it."

---

## Differentiators

Features that set O-Bass apart. Not universally expected, but valued when present.

| Feature | Value Proposition | Complexity | O-Bass Recommendation |
|---------|-------------------|------------|----------------------|
| **Clean/Colored mode switch** | Versatility without parameter bloat; clean for transparent work, colored for character | Medium | **YES - Core differentiator per PROJECT.md** |
| **Phase-coherent processing** | Many competitors have phase issues; Denise Bass XXL markets "zero phasing" as key feature | High | **YES - Quality differentiator** |
| **Even/odd harmonic blend** | Control over harmonic character (even = warm/tube, odd = aggressive/transistor) | Medium | **Consider for Colored mode** - could be single knob |
| **Pre-delay/position control** | Helps avoid phase clash with original signal (1-2ms range) | Low | **Consider** - single knob, big impact |
| **Sidechain-aware detection** | Better fundamental detection on complex material | High | **NO for v1** - per PROJECT.md out of scope |
| **Oversampling** | Cleaner harmonic generation, less aliasing | Medium | **YES** - expected quality standard |
| **Dry/wet mix** | Parallel processing built-in | Low | **Consider** - only if UI budget allows |

**Competitive positioning:**
- R-Bass: Minimal controls, quick results, sounds "polished"
- MaxxBass: More controls (dynamics, slope), visual feedback
- Bass XXL: Markets phase-free processing, harmonic slope control
- Precision Enhancer Hz: Multiple modes for different sources, pristine quality

O-Bass differentiator should be: **Clean/Colored modes in minimal interface with phase-coherent processing**

---

## Anti-Features

Features to deliberately NOT build. These would violate the minimal UI constraint or add complexity without proportional value.

| Anti-Feature | Why Avoid | What Competitors Do | O-Bass Alternative |
|--------------|-----------|---------------------|-------------------|
| **Multi-band crossover controls** | Adds 3+ knobs; requires expertise to use well; violates minimal philosophy | bx_subsynth has 3 bands with individual controls | Single intelligent frequency targeting |
| **Built-in compressor section** | Scope creep; users have dedicated compressors; adds 4+ controls | MaxxBass has dynamics section | Let users chain with OuariconComp |
| **Spectrum analyzer** | Encourages visual mixing over listening; adds UI complexity; not needed for "results" approach | Some plugins show before/after spectrum | None—trust your ears |
| **Filter slope control** | Marginal benefit vs added knob; can be baked into algorithm | MaxxBass has adjustable slope | Fixed slope per mode (clean vs colored) |
| **Solo harmonics feature** | Useful for learning but not for results; adds button | MaxxBass has harmonic solo | None |
| **MIDI frequency control** | Niche use case; adds input complexity | Bass XXL supports MIDI note input | Manual frequency knob is sufficient |
| **Subharmonic synthesis** | Different technique entirely; requires different algorithm and more controls; phase complexity | bx_subsynth, Waves LoAir, Submarine | Psychoacoustic only—generates upper harmonics |
| **Mid/side processing** | Per PROJECT.md out of scope; adds complexity | Some mastering-focused plugins | Stereo processing only |
| **Multiple saturation modes** | Feature creep; clean/colored covers the need | Saturation plugins offer tube/tape/transistor | Two modes: clean + colored |
| **Preset browser in main UI** | Clutters minimal interface | Some plugins have prominent preset sections | Minimal preset access, or use OuariconPresetManager pattern |

**Key principle:** Every control not added is a decision point users don't have to think about. For "results over tweaking" philosophy, fewer controls = faster results.

---

## Feature Dependencies

```
Target Frequency Control
    |
    v
Harmonic Generation Algorithm
    |
    +---> Clean Mode (transparent harmonics)
    |
    +---> Colored Mode (saturated harmonics)
    |
    v
Intensity Control
    |
    v
Output Gain
```

**Critical path:** The harmonic generation algorithm is the core. Everything else wraps around it. Clean/colored mode affects HOW harmonics are generated, not IF.

---

## MVP Recommendation

For O-Bass v1.0 with 3-5 control constraint:

### Primary Controls (Always Visible)

1. **Frequency** - Target frequency for enhancement (40-200Hz range)
2. **Enhance** - Amount of harmonic generation (intensity)
3. **Output** - Gain compensation
4. **Mode** - Clean/Colored toggle

That's 3 knobs + 1 toggle = 4 controls. Clean, focused, matches R-Bass simplicity while adding mode switching.

### Secondary Control (If UI Budget Allows)

5. **Mix** - Dry/wet blend for parallel processing

Only add this if user testing shows demand. It's useful but not essential—users can achieve parallel processing via DAW routing.

### Quality Features (Not UI Controls)

- **Phase-coherent algorithm** - Differentiator, no control needed
- **Oversampling** - Quality standard, no control needed (or auto-detect based on mode)
- **0-latency option** - Important for tracking, could be global preference

---

## Use Cases by Feature

| Use Case | Required Features | Ideal Mode |
|----------|-------------------|------------|
| Kick drum enhancement | Frequency ~50-60Hz, moderate enhance | Clean or Colored |
| Bass guitar presence | Frequency ~80-100Hz, subtle enhance | Clean |
| Synth bass on small speakers | Frequency ~40-80Hz, heavy enhance | Clean |
| 808 warmth | Frequency ~50Hz, moderate enhance | Colored |
| Mix bus low-end glue | Frequency ~60-100Hz, subtle enhance | Clean |
| Sound design / creative | Variable frequency, heavy enhance | Colored |

**Insight:** Clean mode likely used 70% of time (transparent enhancement), Colored mode for character/creative work.

---

## Competitive Matrix

| Plugin | Controls | Modes | Price | Strengths | Weaknesses |
|--------|----------|-------|-------|-----------|------------|
| Waves R-Bass | 3 | 1 | $29 | Simple, quick | No character options |
| Waves MaxxBass | 7+ | 1 | $29 | Visual feedback, dynamics | Complex for simple task |
| Denise Bass XXL | 4 | 1 | $49 | Phase-free, slope control | Single character |
| UAD Precision Hz | 4 | 4 | $149 | Quality, source-specific modes | UAD-only, expensive |
| Slate Infinity Bass | 4 | 4 | Sub | Multiple characters | Subscription model |
| Fire Boy | 3 | 1 | FREE | Dead simple | Basic features |

**O-Bass positioning:** R-Bass simplicity + mode switching + phase-coherent quality. Price TBD but competitive with Bass XXL tier (~$49-79).

---

## Sources

### HIGH Confidence (Official/Product Sources)
- [Waves MaxxBass Product Page](https://www.waves.com/plugins/maxxbass) - Feature specifications
- [Waves Bass Plugins Comparison](https://www.waves.com/bass-plugins-and-sub-enhancers-compared) - R-Bass vs MaxxBass detailed comparison
- [Denise Audio Bass XXL](https://www.deniseaudio.com/plugins/bass-xxl) - Phase-free marketing, control layout
- [Universal Audio Precision Enhancer Hz Manual](https://help.uaudio.com/hc/en-us/articles/33536774357780-Precision-Enhancer-Hz-Manual) - Mode specifications, frequency ranges
- [Plugin Alliance bx_subsynth](https://www.plugin-alliance.com/en/products/bx_subsynth.html) - Multi-band subharmonic approach
- [Slate Digital Infinity Bass](https://slatedigital.com/infinity-bass-plugin/) - Mode-based interface design

### MEDIUM Confidence (Industry Publications)
- [Pro Audio Files - 7 Favorite Bass Enhancer Plugins](https://theproaudiofiles.com/bass-enhancer-plugins/) - Professional use cases and tips
- [Artists in DSP - 7 Best VST Plugins for Sub Bass 2026](https://artistsindsp.com/the-7-best-vst-plugins-for-powerful-sub-bass-2026/) - Current market overview
- [MATLAB Psychoacoustic Bass Enhancement](https://www.mathworks.com/help/audio/ug/psychoacoustic-bass-enhancement-for-band-limited-signals.html) - Algorithm fundamentals
- [Mastering The Mix - Mix Bus Processing](https://www.masteringthemix.com/blogs/learn/everything-you-need-to-know-about-mix-bus-processing) - Use case guidance

### LOW Confidence (Forum Discussions)
- [KVR Audio - Bass Enhancement Discussion](https://www.kvraudio.com/forum/viewtopic.php?t=612874) - User preferences
- [Gearspace - Bass Enhancement Threads](https://gearspace.com/) - Professional opinions

---

## Implications for Roadmap

1. **Phase 1 (Core):** Implement psychoacoustic algorithm with single mode first—prove the core works
2. **Phase 2 (Modes):** Add clean/colored mode switching—the differentiator
3. **Phase 3 (Polish):** Phase coherence optimization, oversampling, preset system
4. **Phase 4 (UI):** WebView interface with 4-control layout

**Research flags:**
- Algorithm design will need dedicated research (psychoacoustic literature, harmonic generation techniques)
- Clean vs colored mode implementation—what specifically makes "colored" sound colored?
- Phase coherence techniques—how does Denise achieve "zero phasing"?

---

*Research conducted for O-Bass v1.0 milestone. Feature recommendations align with minimal UI constraint and "results over tweaking" philosophy.*
