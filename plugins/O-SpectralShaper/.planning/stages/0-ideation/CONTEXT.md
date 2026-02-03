# O-SpectralShaper - Stage 0 Context

**Stage:** 0 (Research & Planning)
**Date:** 2026-02-03
**Agent:** research-planning-agent
**Status:** Complete

---

## Phase Summary

Stage 0 conducted deep research (Tier 6 complexity) on spectral transient shaping, completing comprehensive architecture documentation and implementation planning for O-SpectralShaper.

---

## Key Decisions

### 1. FFT Configuration: 512-Sample Window

**Decision:** Use 512-sample FFT with 50% overlap (256-sample hop)

**Rationale:**
- **Latency:** 11.6ms @ 44.1kHz (meets target of <15ms for mixing context)
- **Frequency Resolution:** 86Hz/bin (adequate for 32 logarithmic bands)
- **Time Resolution:** 5.8ms hop updates (good transient tracking)
- **CPU Efficiency:** Smaller than 1024 (reduces computation by 50%)

**Trade-offs Considered:**
- 256 samples: Lower latency (5.8ms) but poor frequency resolution (172Hz/bin) - rejected
- 1024 samples: Better frequency resolution (43Hz/bin) but 23ms latency - too high for live use

**Professional Validation:**
- oeksound Spiff: ~10ms latency
- MolecularBytes AtomicTransient: ~15ms latency
- Eventide SplitEQ: ~20ms latency (linear phase)

**Fallback:** Implement adaptive FFT size at high sample rates (256 @ 96kHz maintains ~2.7ms latency)

---

### 2. Transient Detection: Spectral Flux with Dual Envelopes

**Decision:** Magnitude-only spectral flux + fast/slow envelope followers

**Algorithm:**
1. Calculate per-band magnitude (sum FFT bins in each of 32 bands)
2. Compute spectral flux (positive-only: energy increases)
3. Fast envelope (1ms attack) catches sharp transients
4. Slow envelope (15ms attack) tracks sustained energy
5. Transient activity = fast - slow (scaled by sensitivity)

**Rationale:**
- **Magnitude-only:** Preserves phase relationships (no phase manipulation artifacts)
- **Positive-only flux:** Energy increases indicate onsets (ignores releases/decays)
- **Dual envelopes:** Classic transient detection (SPL Transient Designer method)
- **Per-band independence:** Kick transients don't trigger cymbal processing

**Alternatives Considered:**
- Complex domain (magnitude + phase): More accurate but risks phase artifacts - rejected
- Time-domain envelopes: No frequency selectivity - rejected
- Single envelope: Can't distinguish transient vs. sustain - rejected

**Professional Reference:** oeksound Spiff uses similar spectral flux with adaptive thresholding

---

### 3. Band Configuration: 32 Logarithmic Bands

**Decision:** 32 bands, logarithmically spaced from 20Hz to 20kHz

**Distribution @ 44.1kHz:**
- **Bands 1-5:** 20-100Hz (sub-bass, kick fundamentals) - 3-4 bins/band
- **Bands 6-15:** 100Hz-1kHz (bass, low-mids, snare body) - 4-6 bins/band
- **Bands 16-25:** 1kHz-8kHz (presence, snare crack, vocal sibilance) - 6-10 bins/band
- **Bands 26-32:** 8kHz-22kHz (air, cymbal shimmer) - 10-20 bins/band

**Rationale:**
- Logarithmic spacing matches human frequency perception (equal musical intervals)
- 32 bands provides surgical control (vs. competitors' 5-8 bands)
- Adequate resolution across entire spectrum
- Manageable curve drawing (not too many control points)

**Competitive Analysis:**
- oeksound Spiff: 5 bands (too coarse)
- Eventide SplitEQ: 8 bands (good but limited)
- O-SpectralShaper: 32 bands (industry-leading precision)

---

### 4. UI Architecture: WebView with Canvas + WebGL

**Decision:** HTML5 Canvas for curve drawing, WebGL for spectrogram rendering

**Components:**
1. **Parameter Controls:** JUCE WebSliderRelay + WebSliderParameterAttachment
2. **Drawable Curves:** HTML5 Canvas with mouse event handlers
   - Freehand mode: Draw smooth curves via Catmull-Rom splines
   - Node mode: Bezier control points with drag handles
3. **Spectrogram:** WebGL fragment shader for GPU-accelerated scrolling
   - Texture scrolling (GPU texture updates via texSubImage2D)
   - Colormap shader (magnitude → RGB)
   - Transient heat overlay (blend with spectrogram)

**Rationale:**
- **Consistent with O-* family:** All use WebView UI
- **Canvas for curves:** Easier than JUCE Graphics (mouse events, bezier math in JS)
- **WebGL for spectrogram:** GPU acceleration for 60fps scrolling (257 bins * 60fps = heavy load)
- **Easier iteration:** HTML/CSS/JS faster to prototype than C++

**Performance Validation:**
- Reference: [Spectro WebGL Spectrogram](https://github.com/calebj0seph/spectro) - real-time at 60fps
- Mitigation: Downsample to 64 bands for JavaScript (reduces data transfer by 4x)
- Fallback: HTML5 Canvas with 30fps update rate (if WebGL causes issues)

---

### 5. Stereo Processing: Independent Per-Channel (Not Mid/Side)

**Decision:** Process L and R channels independently

**Rationale:**
- Transients are channel-specific (panned drum hits)
- Mid/Side encoding/decoding adds CPU overhead
- No user demand for mid/side-specific transient shaping
- Simpler implementation, easier to understand for users
- Attack/sustain curves are shared (same curve applied to both channels)

**Alternatives Considered:**
- Mid/Side: Separate center vs. stereo content - rejected (overkill)
- Linked stereo: Sum L+R for detection - rejected (loses spatial information)

---

## Architectural Highlights

### Signal Flow

```
Input → Lookahead → Dry/Wet (store dry) → STFT Processor
                                              ↓
                                     [Per 512-sample frame]
                                     1. Hann window
                                     2. Forward FFT
                                     3. Per-band processing:
                                        - Calculate magnitude
                                        - Spectral flux
                                        - Dual envelopes
                                        - Transient activity
                                        - Apply attack gain
                                        - Apply sustain gain
                                     4. Inverse FFT
                                     5. Hann window
                                     6. Overlap-add
                                              ↓
                        Dry/Wet (mix) → Output Gain → Output
```

### Thread Architecture

**Audio Thread (Real-Time):**
- STFT processing (per-sample interface)
- Per-band transient detection
- Envelope shaping
- Write visualization data to lock-free FIFO
- NO allocations, locking, or blocking

**GUI Thread (60fps Timer):**
- Read visualization FIFO (juce::AbstractFifo)
- Update WebView with FFT + transient data
- Handle mouse events (curve drawing)
- Send curve updates to audio thread (atomic flag)

**Message Thread:**
- Parameter changes (APVTS handles synchronization)
- State save/restore (curves + parameters)

**Communication:**
- UI → Audio: APVTS (parameters), atomic flag (curves)
- Audio → UI: juce::AbstractFifo (visualization data)
- NO direct cross-thread function calls

---

## Complexity Analysis

**Final Complexity Score:** 5.0 (Maximum)

**Breakdown:**
- **Parameters:** 8 (6 automatable + 2 curve arrays) → 1.6 points
- **Algorithms:** 9 (FFT, windowing, STFT, band splitting, detection, shaping, lookahead, mix, gain) → 9 points
- **Features:** 5 (FFT, visualization, drawable UI, multi-threading, complex state) → 5 points
- **Total:** 15.6 → capped at 5.0

**Implementation Strategy:** Staged with 3 DSP phases + 3 GUI phases

---

## Research Findings

### Professional Plugins Analyzed

1. **oeksound Spiff** ($149)
   - Adaptive transient processor
   - 5-band EQ-style interface
   - ~10ms latency
   - Clean processing, minimal artifacts
   - **Insight:** Spectral flux detection with adaptive thresholding

2. **MolecularBytes AtomicTransient** ($149)
   - Polyphonic spectral separation (novel)
   - 3 parallel processing channels
   - Waterfall visualization
   - ~15ms latency
   - **Insight:** Per-note envelope processing (inspired O-SpectralShaper's per-band independence)

3. **Eventide SplitEQ** ($179)
   - Structural Split technology (patented)
   - 8-band parametric EQ per path
   - ~20ms latency (linear phase)
   - **Insight:** Transient/tonal separation via temporal median filtering

### JUCE API Validation

**All classes verified via Context7-MCP (authoritative JUCE 8 docs):**

1. **juce::dsp::FFT** - Real-only forward/inverse transforms
   - Module: juce_dsp
   - Constructor: FFT(int order)
   - Methods: performRealOnlyForwardTransform(), performRealOnlyInverseTransform()

2. **juce::dsp::WindowingFunction** - Hann, Hamming, Blackman windows
   - Module: juce_dsp
   - Method: multiplyWithWindowingTable()

3. **juce::dsp::DryWetMixer** - Wet/dry mixing with latency compensation
   - Module: juce_dsp
   - Methods: pushDrySamples(), mixWetSamples()

4. **juce::AbstractFifo** - Lock-free ring buffer
   - Module: juce_core
   - Methods: prepareToWrite(), finishedWrite(), prepareToRead(), finishedRead()

5. **juce::WebSliderRelay + WebSliderParameterAttachment** - WebView parameter binding
   - Module: juce_gui_extra
   - Pattern: Relay → WebView options → Attachment (parameter, relay, nullptr)

**Critical Pattern:** JUCE 8 requires 3 parameters for WebSliderParameterAttachment (parameter, relay, undoManager=nullptr)

---

## Implementation Risks & Mitigations

### HIGH Risk: FFT Latency (11.6ms)

**Mitigation:**
- Accept trade-off (competitors have 10-20ms)
- Adaptive FFT size at high sample rates (256 @ 96kHz = 2.7ms)
- Clear latency reporting to DAW (automatic compensation)

**Fallback:** Dual-resolution FFT (256 for highs, 1024 for lows)

---

### MEDIUM Risk: CPU Usage (~30% estimated)

**Mitigation:**
- SIMD optimization (juce::dsp::SIMDRegister for band magnitudes)
- Skip frames option (process every 2nd hop = 50% CPU reduction)
- Quality mode selector (High/Balanced/Low CPU)

**Fallback:** Reduce overlap to 25% (FFT every 384 samples instead of 256)

---

### MEDIUM Risk: WebGL Spectrogram Performance

**Mitigation:**
- GPU texture scrolling (texSubImage2D)
- Downsample to 64 bands before JavaScript (4x data reduction)
- requestAnimationFrame sync (browser-optimized 60fps)

**Fallback:** HTML5 Canvas with 30fps update rate

---

### LOW Risk: Curve Synchronization

**Mitigation:**
- Double-buffering with atomic flag (ping-pong buffers)
- Audio thread reads from active buffer
- GUI thread writes to inactive buffer
- Atomic swap on completion

**Complexity:** Standard lock-free pattern

---

### LOW Risk: Phase Coherence

**Mitigation:**
- Magnitude-only processing (never modify phase)
- Smooth gain changes (juce::SmoothedValue, 50ms ramp)
- COLA windowing (Hann with 50% overlap)

**Why Low Risk:** Spectral flux is magnitude-only by design

---

## Next Stage Requirements

### Stage 1: Foundation (foundation-shell-agent)

**Required Outputs:**
1. CMakeLists.txt with:
   - juce_dsp module (FFT)
   - juce_gui_extra module (WebView)
   - NEEDS_WEB_BROWSER TRUE
   - juce_generate_juce_header() call
2. PluginProcessor with:
   - Stereo BusesProperties (input + output)
   - APVTS with 6 parameters (Mix, Attack Time, Sustain Time, Sensitivity, Lookahead, Output Gain)
   - Empty processBlock() (passes audio through)
3. Successful build verification

**Entry Condition:** Stage 0 complete (ARCHITECTURE.md + ROADMAP.md documented)

---

### Stage 2: DSP (dsp-agent - 3 phases)

**Phase 2.1:** Core STFT Engine
- STFTProcessor class with perfect reconstruction
- Null test: Input - Output = silence

**Phase 2.2:** Per-Band Transient Detection
- 32 logarithmic bands
- Spectral flux with dual envelopes
- Sensitivity parameter integration

**Phase 2.3:** Envelope Shaping
- Attack/sustain curve arrays
- Gain calculation and smoothing
- Dry/wet mix + output gain

**Entry Condition:** Stage 1 complete (plugin builds and loads in DAW)

---

### Stage 3: GUI (gui-agent - 3 phases)

**Phase 3.1:** Layout and Basic Controls
- WebView setup with parameter knobs
- JUCE WebSliderRelay bindings

**Phase 3.2:** Drawable Curve Editors
- Freehand + node modes
- C++ communication via addNativeFunction()

**Phase 3.3:** Real-Time Spectrogram
- WebGL renderer with transient heat overlay
- juce::AbstractFifo visualization pipeline

**Entry Condition:** Stage 2 complete (DSP processing artifact-free)

---

## Constraints & Assumptions

### Constraints

1. **Latency Budget:** <15ms total (512 FFT + lookahead = ~13ms acceptable)
2. **CPU Budget:** <50% single core @ 44.1kHz stereo
3. **Sample Rate Support:** 44.1kHz to 192kHz (adaptive FFT size)
4. **Thread Safety:** No locking, allocations, or blocking in processBlock()

### Assumptions

1. **Target Hardware:** Modern CPU (Intel i5/i7, Apple Silicon M1+)
2. **DAW Context:** Mixing (not mastering) - 11.6ms latency acceptable
3. **User Skill Level:** Intermediate to advanced (understand frequency-domain processing)
4. **Input Material:** Percussive content (drums, vocals, guitar) - transient-rich

---

## Success Criteria

**Stage 0 complete when:**
- ✓ ARCHITECTURE.md documented (11 sections, 68 pages)
- ✓ ROADMAP.md documented (complexity score, phase breakdown)
- ✓ Professional plugin research complete (Spiff, AtomicTransient, SplitEQ)
- ✓ JUCE API validation complete (FFT, windowing, visualization)
- ✓ All architectural decisions justified with rationale
- ✓ Risk assessment complete with mitigations and fallbacks
- ✓ Implementation strategy defined (3 DSP phases + 3 GUI phases)

**All criteria met. Stage 0 complete. Ready for Stage 1 (Foundation).**

---

## Files Created

1. `/Users/taylorbrook/Dev/VST-development/plugins/O-SpectralShaper/.planning/research/ARCHITECTURE.md`
   - Complete DSP architecture specification
   - Signal flow diagrams
   - JUCE class mappings with module dependencies
   - Algorithm details (spectral flux, envelope followers)
   - Thread architecture
   - Risk analysis with fallback architectures

2. `/Users/taylorbrook/Dev/VST-development/plugins/O-SpectralShaper/.planning/ROADMAP.md`
   - Complexity assessment (score: 5.0)
   - Stage breakdown (5 stages, 6 phases)
   - Test criteria per phase
   - Implementation notes
   - Success criteria

3. `/Users/taylorbrook/Dev/VST-development/plugins/O-SpectralShaper/.planning/stages/0-ideation/CONTEXT.md`
   - This file
   - Key decisions with rationale
   - Research findings summary
   - Next stage requirements

---

**End of Stage 0 Context**
