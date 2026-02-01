# O-Freeze - Stage 0 Context (Research & Planning)

**Date:** 2026-02-01
**Stage:** 0 (Ideation - Research & Planning)
**Status:** Complete

---

## Overview

Completed comprehensive research and planning for O-Freeze, a granular freeze effect plugin. The plugin captures audio moments and sustains them indefinitely using overlap-add granular synthesis with Hann windowing for buttery-smooth textures.

**Complexity Assessment:** 5.0 (Complex - maximum score, capped)
**Implementation Strategy:** Phase-based (3 DSP phases, 2 GUI phases)

---

## Key Research Findings

### 1. Granular Synthesis is Industry Standard for Freeze Effects

**Professional plugins researched:**
- Unfiltered Audio Freeze/Sandman Pro - Grain-based freeze with manual trigger
- AudioThing Frostbite 2 - Spectral + granular freeze modes
- Sinevibes Albedo - Up to 64 grains for ultra-smooth texture
- Delta Sound Labs Stream - Delay line freezing with granular extraction

**Key insight:** NO professional freeze plugins use simple buffer looping (all use granular synthesis with windowing). This validates creative brief requirement for "buttery-smooth windowing."

---

### 2. Hann Window is Optimal for Grain Envelope

**Research conclusion:**
- Hann window provides zero-crossing at start/end (no clicks)
- Requires 75% overlap (4 simultaneous grains) for artifact-free summing
- Simpler than Blackman window (single cosine term vs. 3 terms)
- Hamming window REJECTED (doesn't reach zero at endpoints → clicks)

**Implementation decision:** Pre-compute Hann window in prepareToPlay(), use lookup table in audio thread (zero runtime calculation).

---

### 3. Threshold Gate Requires Hysteresis

**Research finding:** Simple threshold triggers cause rapid on/off cycling (fluttering) when input level hovers near threshold.

**Solution:** 3dB hysteresis gap
- Freeze engages at threshold (e.g., -40dB)
- Freeze releases at threshold + 3dB (e.g., -37dB)
- RMS averaging over 20ms window prevents false triggers on transients

**Alternative considered:** Peak detection (simpler) - Rejected due to transient sensitivity

---

### 4. JUCE Has No Built-In Granular Synthesis Class

**JUCE API research:**
- juce::dsp::Reverb, juce::dsp::Delay, juce::dsp::Chorus - All present
- juce::dsp::GranularEngine - Does NOT exist
- Custom implementation required using juce::AudioBuffer + juce::Random

**Implementation approach:**
- Manual grain scheduling (trigger new grain every 12.5ms for 75% overlap)
- Hann windowing via lookup table (pre-computed)
- Overlap-add summing with normalization (divide by grain count)

---

### 5. Complexity Score Justification

**Calculation:**
- Parameters: 5 (FREEZE, THRESHOLD, MODE, DRIFT, MIX) = 1.0 point
- Algorithms: 3 DSP components (Granular Engine, Threshold Gate, Crossfade) = 3 points
- Features: Custom granular synthesis (+1), Modulation system (+1) = 2 points
- **Total:** 1.0 + 3 + 2 = 6.0 → capped at 5.0

**Tier:** Complex (≥ 3.0) → Phase-based implementation required

**Key complexity drivers:**
1. Custom granular engine (no JUCE class, overlap-add implementation required)
2. Grain scheduling logic (timing, active grain management)
3. Threshold gate state machine (RMS detection + hysteresis)

---

## Architecture Decisions

### Decision 1: Granular Synthesis vs. Simple Buffer Loop

**Chosen:** Overlap-add granular synthesis with Hann windowing

**Rationale:**
- Creative brief explicitly requests "buttery-smooth windowing" and "grains should blend seamlessly"
- ALL professional freeze plugins use granular approach (industry standard)
- Simple looping produces audible loop seam (click at loop point) - Not acceptable quality

**Tradeoff accepted:** Higher CPU (~15% vs ~5% for simple loop) - Acceptable for studio/creative use

---

### Decision 2: Fixed 2-Second Buffer vs. Adjustable Buffer

**Chosen:** Fixed 2-second freeze buffer

**Rationale:**
- Balances quality (long texture) with memory (~750KB)
- Industry norm (professional plugins use 1-3 second buffers)
- Simpler implementation (no dynamic resizing, no UI parameter)

**Alternative rejected:** User-adjustable buffer (0.5s to 5s) - Adds UI complexity with minimal user benefit

---

### Decision 3: Manual + Threshold in Single Plugin

**Chosen:** Single plugin with MODE parameter (Manual vs. Threshold)

**Rationale:**
- Creative brief explicitly requests "Two freeze modes provide flexibility"
- Standard pattern in gate/expander plugins (mode switching common)
- Simpler distribution (one plugin, not two SKUs)

**UI approach:** Show/hide controls based on MODE (FREEZE button disabled in Threshold mode, THRESHOLD knob disabled in Manual mode)

---

### Decision 4: 4 Grains (75% Overlap) for Balance

**Chosen:** 4 simultaneous grains with 75% overlap

**Rationale:**
- Minimum grain count for artifact-free Hann window overlap (research: 75% overlap required)
- Balance between smoothness and CPU cost
- Professional plugins use 4-8 grains (Sinevibes Albedo uses 64, but overkill for static freeze)

**Fallback options:**
- 2 grains (50% overlap) - Lower CPU (~8%), acceptable quality, more amplitude modulation
- 8 grains (87.5% overlap) - Smoother texture, higher CPU (~25%), may be needed if 4-grain shows artifacts

---

## Implementation Constraints

### Constraint 1: No Real-Time Memory Allocation

**Impact:** Freeze buffer must be pre-allocated in prepareToPlay()
**Mitigation:** Fixed 2-second buffer size (96000 samples @ 48kHz stereo = ~750KB)
**Validation:** Use juce::AudioBuffer (manages memory automatically, bounds-checked in debug)

---

### Constraint 2: Zero-Latency Passthrough (When Not Frozen)

**Impact:** Granular engine must be bypassed completely when freeze inactive
**Mitigation:** Conditional processing (skip granular engine if freeze_state == false)
**Validation:** CPU profiler - verify 0% CPU usage in passthrough mode

---

### Constraint 3: Thread-Safe Parameter Access

**Impact:** FREEZE button state must be atomic (UI thread writes, audio thread reads)
**Mitigation:** Use std::atomic<bool> for FREEZE button state
**Validation:** ThreadSanitizer (verify no data races between UI and audio threads)

---

## Risk Assessment

### High-Risk Component: Granular Engine

**Risk factors:**
1. Overlap-add artifacts if grain overlap insufficient (< 75%)
2. Window function discontinuities cause clicks
3. Grain timing jitter causes unstable texture

**Mitigation strategy:**
1. Implement simple buffer loop first (validate freeze trigger logic)
2. Replace with granular engine incrementally (1 grain → 2 grains → 4 grains)
3. Pre-compute Hann window (no runtime calculation errors)
4. Use fixed grain size (50ms) to simplify scheduling

**Fallback architecture:**
- Primary: 4-grain overlap-add with Hann window (highest quality)
- Fallback 1: 8 grains (if artifacts unacceptable)
- Fallback 2: 2 grains (if CPU too high)
- Fallback 3: Simple buffer loop with crossfade (if granular fails entirely)

---

### Medium-Risk Component: Threshold Gate

**Risk factors:**
1. Insufficient hysteresis causes fluttering (rapid on/off cycling)
2. RMS window too short causes false triggers on transients
3. RMS window too long causes slow response

**Mitigation strategy:**
1. Start with peak detection (simpler, validate state machine logic)
2. Replace with RMS detection after state machine proven
3. Test 3dB hysteresis, adjust to 6dB if needed
4. Test with various input material (drums, pads, ambient)

**Fallback architecture:**
- Primary: RMS-based gate with 3dB hysteresis (smooth, stable)
- Fallback 1: 6dB hysteresis (if 3dB shows fluttering)
- Fallback 2: Peak detection (if RMS too complex)

---

## Next Steps (Stage 1: Foundation)

1. **Create plugin project structure** (CMakeLists.txt, PluginProcessor skeleton)
   - Use juce_add_plugin with correct metadata (COMPANY_NAME, PLUGIN_CODE)
   - Configure BusesProperties (stereo input, stereo output)
   - Add juce_generate_juce_header() (JUCE 8 requirement - see juce8-critical-patterns.md #1)

2. **Implement APVTS parameters** (THRESHOLD, DRIFT, MIX, MODE, FREEZE)
   - THRESHOLD: Float, -60 to 0 dB, default -40 dB, linear dB scale
   - DRIFT: Float, 0 to 100%, default 0%, linear scale
   - MIX: Float, 0 to 100%, default 100%, linear scale
   - MODE: Choice, {Manual, Threshold}, default Manual
   - FREEZE: Bool, default false (NOT persisted in state)

3. **Set up build system** (verify ninja build, test in DAW)
   - Build VST3 and AU targets
   - Install to system folders (~/Library/Audio/Plug-Ins/)
   - Clear DAW caches (see CLAUDE.md critical cache clearing steps)
   - Verify plugin loads in Logic Pro or Ableton

4. **Validate foundation before DSP** (plugin loads, parameters accessible)
   - Test parameter automation in DAW
   - Verify passthrough audio (input → output, no processing yet)
   - Check CPU usage (should be near 0% in passthrough)

---

## Files Generated

1. **ARCHITECTURE.md** - Complete DSP specification
   - 11 required sections (Core Components, Processing Chain, System Architecture, etc.)
   - Every feature from research documented
   - JUCE module dependencies mapped
   - Fallback architectures for high-risk components
   - Processing order requirements

2. **ROADMAP.md** - Implementation plan
   - Complexity score breakdown (5.0)
   - Phase-based strategy (3 DSP phases, 2 GUI phases)
   - Test criteria per phase
   - Performance estimates and optimization notes

3. **CONTEXT.md** (this file) - Research findings and decisions
   - Key insights from professional plugin research
   - Architecture decision rationale
   - Risk assessment and mitigation strategies
   - Next steps for Stage 1

---

## Lessons Learned

1. **Granular synthesis is non-trivial:** No JUCE built-in, requires custom overlap-add implementation. Plan for incremental development (simple loop → granular).

2. **Window function choice matters:** Hann window is optimal (zero-crossing, simple computation). Hamming window would cause clicks.

3. **Hysteresis is critical for gates:** Simple threshold triggers cause fluttering. 3dB hysteresis gap is industry standard.

4. **Professional plugins set quality bar:** ALL freeze plugins use granular synthesis (no simple looping found). Creative brief requirement for "buttery-smooth" is achievable but requires careful implementation.

5. **Complexity score reflects risk:** Score of 5.0 (maximum) driven by custom granular engine + state machine + modulation. Phase-based implementation is necessary (not optional).

---

## Sources

- [12 Best Granular Effect Plugins 2026](https://pluginoise.com/12-best-granular-effect-plugins/)
- [Unfiltered Audio Sandman – Plugin Alliance](https://www.plugin-alliance.com/en/products/unfiltered_audio_sandman.html)
- [AudioThing Frostbite 2](https://www.audiothing.net/effects/frostbite/)
- [Granular Synthesis Module - DSP Concepts](https://documentation.dspconcepts.com/awe-designer/8.D.2.6/granular-synthesis-module)
- [Audio Synthesis - Window Functions](https://michaelkrzyzaniak.com/AudioSynthesis/2_Audio_Synthesis/11_Granular_Synthesis/1_Window_Functions/)
- Curtis Roads, "Microsound" (2001) - Granular synthesis theory
- JUCE Documentation (/juce-framework/juce) - AudioBuffer, LinearSmoothedValue, DryWetMixer APIs
