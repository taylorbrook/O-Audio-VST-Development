# O-SpectralShaper - Implementation Roadmap

---
**Plugin:** O-SpectralShaper
**Type:** Audio Effect (Spectral Transient Shaper)
**Created:** 2026-02-03
**Status:** Ready for Implementation
---

## Complexity Assessment

### Complexity Score Calculation

**Formula:**
```
score = min(param_count / 5, 2.0) + algorithm_count + feature_count
Cap at 5.0
```

#### Parameter Analysis

**From REQUIREMENTS.md:**
- Mix (P1)
- Attack Time (P2)
- Sustain Time (P3)
- Sensitivity (P4)
- Lookahead (P5)
- Output Gain (P6)
- Attack Curve [32 values] (P7-P38) - Counted as 1 composite parameter
- Sustain Curve [32 values] (P39-P70) - Counted as 1 composite parameter

**Total Parameters:** 8

**Parameter Score:**
```
param_score = min(8 / 5, 2.0) = min(1.6, 2.0) = 1.6
```

#### Algorithm Analysis

**From ARCHITECTURE.md - Core Components:**
1. FFT Analysis Engine (juce::dsp::FFT)
2. Windowing Function (juce::dsp::WindowingFunction)
3. Overlap-Add STFT Processor
4. Band Splitting (32 logarithmic bands)
5. Per-Band Transient Detection (spectral flux + dual envelopes)
6. Envelope Shaping (attack/sustain curves)
7. Lookahead Buffer
8. Dry/Wet Mix (juce::dsp::DryWetMixer)
9. Output Gain (smoothed)

**Total Algorithms:** 9

#### Feature Analysis

**Complexity Features:**

| Feature | Score | Detection |
|---------|-------|-----------|
| FFT/frequency domain | +1 | juce::dsp::FFT, STFT, spectral processing |
| Real-time visualization | +1 | Spectrogram + transient heat overlay at 60fps |
| Drawable UI curves | +1 | Freehand + node editing modes for 32 bands |
| Multi-threaded processing | +1 | Audio thread + GUI thread + visualization FIFO |
| Complex state management | +1 | 64 curve values + 6 params + WebView sync |

**Total Feature Score:** 5

#### Final Complexity Score

```
total_score = 1.6 + 9 + 5 = 15.6
final_score = min(15.6, 5.0) = 5.0
```

**Classification:** COMPLEX (score = 5.0)

**Strategy:** Phase-based implementation with test criteria per phase

---

## Implementation Strategy

### Decision: Staged Implementation

**Rationale:**
- **Complexity Score = 5.0** (maximum complexity)
- **FFT Processing:** Novel per-band transient detection (not standard JUCE pattern)
- **Real-Time Visualization:** Spectrogram + heat overlay requires GPU optimization
- **Drawable Curves:** Custom UI pattern with 32-band synchronization
- **Thread Complexity:** 3-way communication (audio, GUI, visualization)

**Approach:**
- Break Stage 1 (DSP) into 3 phases
- Break Stage 2 (GUI) into 3 phases
- Each phase gets git commit + validation
- Clear test criteria before moving to next phase

---

## Stage Breakdown

### Stage 0: Research & Planning (COMPLETE)
**Status:** ✓ Complete
**Duration:** 30 minutes (Deep Research - Tier 6 complexity)

**Deliverables:**
- ✓ ARCHITECTURE.md (complete DSP specification)
- ✓ ROADMAP.md (this document)
- ✓ Complexity assessment (score: 5.0)
- ✓ Professional plugin research (Spiff, AtomicTransient, SplitEQ)
- ✓ JUCE API validation (FFT, windowing, visualization)

---

### Stage 1: Foundation (Build System & Parameters)
**Agent:** foundation-shell-agent
**Duration:** ~20 minutes
**Single Pass:** No phases (straightforward CMake + parameter setup)

**Deliverables:**
1. CMakeLists.txt
   - juce_add_plugin configuration
   - juce_dsp module dependency (for FFT)
   - juce_gui_extra module dependency (for WebView)
   - NEEDS_WEB_BROWSER TRUE flag
   - juce_generate_juce_header() call
2. PluginProcessor.h/cpp skeleton
   - BusesProperties (stereo input/output)
   - APVTS with 6 parameters (Mix, Attack Time, Sustain Time, Sensitivity, Lookahead, Output Gain)
   - Empty processBlock() (passes audio through)
3. Build verification (compiles successfully)
4. Git commit

**Test Criteria:**
- ✓ Plugin builds without errors
- ✓ Loads in DAW (Logic Pro / Ableton)
- ✓ Parameters visible in DAW automation
- ✓ Audio passes through (no silence)

---

### Stage 2: DSP Implementation (Spectral Processing)
**Agent:** dsp-agent
**Duration:** ~60 minutes (COMPLEX - 3 phases)

#### Phase 2.1: Core STFT Engine

**Objective:** Implement overlap-add FFT processing with perfect reconstruction

**Implementation:**
1. STFTProcessor class
   - juce::dsp::FFT (forward + inverse)
   - juce::dsp::WindowingFunction (Hann)
   - Input/output FIFOs (512 samples each)
   - processSample() method (sample-by-sample interface)
2. Integrate into PluginProcessor::processBlock()
   - Per-channel STFT processing
   - Latency reporting (setLatencySamples(512))
3. Bypass mode (copy input → output without modification)

**Test Criteria:**
- ✓ Audio passes through without artifacts
- ✓ Null test: Input - Output = silence (perfect reconstruction)
- ✓ No phase distortion (mono sum test)
- ✓ Latency compensation works in DAW (aligned with other tracks)

**Git Commit:** "feat(O-SpectralShaper): Phase 2.1 - Core STFT engine with perfect reconstruction"

---

#### Phase 2.2: Per-Band Transient Detection

**Objective:** Detect transients independently in 32 frequency bands

**Implementation:**
1. Band splitting
   - setupBandBoundaries() (logarithmic 20Hz-20kHz)
   - 32 bands, adaptive to sample rate
2. Spectral flux detection
   - Per-band magnitude calculation
   - Positive-only flux (energy increases)
   - Dual envelope followers (fast 1ms, slow 15ms)
3. Sensitivity parameter integration
   - Scale transient activity by sensitivity (0.0-1.0)
4. Debug output
   - Log transient activities to console (validate detection)

**Test Criteria:**
- ✓ Impulse input triggers transient detection in all bands
- ✓ Sine wave input shows low transient activity (sustain only)
- ✓ Drum loop shows high transient activity at hits, low between hits
- ✓ Sensitivity parameter modulates detection threshold

**Git Commit:** "feat(O-SpectralShaper): Phase 2.2 - Per-band transient detection with spectral flux"

---

#### Phase 2.3: Envelope Shaping & Parameters

**Objective:** Apply attack/sustain curves to shape transients

**Implementation:**
1. Attack/sustain curve arrays
   - std::array<float, 32> attackCurve (default: 0.0 = no change)
   - std::array<float, 32> sustainCurve (default: 0.0 = no change)
2. Gain calculation
   - attackGain = curve * attackTime * transientActivity
   - sustainGain = curve * sustainTime * (1 - transientActivity)
   - Apply to FFT bins in band
3. Parameter integration
   - Attack Time, Sustain Time parameters
   - Lookahead buffer (0-10ms configurable)
4. Dry/Wet mix
   - juce::dsp::DryWetMixer integration
   - Mix parameter (0-100%)
5. Output gain
   - juce::SmoothedValue for smooth gain changes
   - Output Gain parameter (-12 to +12 dB)

**Test Criteria:**
- ✓ Set attackCurve[all] = +1.0 → Drum transients boosted (louder attack)
- ✓ Set sustainCurve[all] = -1.0 → Drum tails reduced (shorter decay)
- ✓ Mix parameter blends dry/wet smoothly (no clicks)
- ✓ Output gain compensates for level changes
- ✓ Lookahead parameter reduces pre-ringing on sharp transients

**Git Commit:** "feat(O-SpectralShaper): Phase 2.3 - Envelope shaping with attack/sustain curves"

---

**Stage 2 Complete When:**
- All 3 phases pass test criteria
- Audio processing is artifact-free (no clicks, pops, phase issues)
- Parameters respond correctly to changes
- CPU usage <50% single core @ 44.1kHz stereo

---

### Stage 3: GUI Implementation (WebView + Visualization)
**Agent:** gui-agent
**Duration:** ~90 minutes (COMPLEX - 3 phases)

#### Phase 3.1: Layout and Basic Controls

**Objective:** WebView setup with parameter knobs/sliders

**Implementation:**
1. PluginEditor.h/cpp
   - juce::WebBrowserComponent setup
   - WebView resource provider (serve HTML/CSS/JS)
   - NEEDS_WEB_BROWSER TRUE in CMakeLists.txt
2. HTML/CSS layout (index.html)
   - Ouaricon dark theme
   - 6 parameter controls (Mix, Attack Time, Sustain Time, Sensitivity, Lookahead, Output Gain)
   - Placeholder areas for spectrogram + curves
3. Parameter binding
   - juce::WebSliderRelay for each parameter
   - juce::WebSliderParameterAttachment to APVTS
   - JavaScript: Juce.getSliderState() for each parameter
4. Basic interaction
   - Knob drag updates parameters
   - DAW automation updates knobs

**Test Criteria:**
- ✓ WebView loads without errors
- ✓ All 6 parameters visible and styled correctly
- ✓ Dragging knobs changes DSP parameters (audible effect)
- ✓ DAW automation moves knobs in UI
- ✓ Preset changes update knobs correctly

**Git Commit:** "feat(O-SpectralShaper): Phase 3.1 - WebView layout with parameter controls"

---

#### Phase 3.2: Drawable Curve Editors

**Objective:** Freehand + node editing for attack/sustain curves

**Implementation:**
1. Canvas curve editor (HTML5 Canvas)
   - Two canvases: Attack (top), Sustain (bottom)
   - X-axis: Frequency (log scale, 32 bands)
   - Y-axis: Boost/cut (-1.0 to +1.0)
2. Freehand mode
   - Mouse drag draws curve
   - Catmull-Rom spline smoothing
   - Sample curve at 32 band centers
3. Node mode
   - Click to place control points
   - Drag handles for bezier precision
   - Interpolate between nodes
4. C++ communication
   - JavaScript: Juce.getNativeFunction("setAttackCurve")
   - C++: addNativeFunction() callback
   - Update processorRef.attackCurve[32] array
   - Atomic flag for thread-safe curve updates
5. Visual feedback
   - Grid overlay (frequency labels, dB scale)
   - Curve color coding (attack = blue, sustain = orange)

**Test Criteria:**
- ✓ Draw attack curve → Transients boost/cut in real-time
- ✓ Draw sustain curve → Decay tails boost/cut in real-time
- ✓ Freehand mode draws smooth curves (no jagged lines)
- ✓ Node mode allows precise control point placement
- ✓ Curves persist across preset save/load
- ✓ No audio glitches when drawing curves

**Git Commit:** "feat(O-SpectralShaper): Phase 3.2 - Drawable attack/sustain curve editors"

---

#### Phase 3.3: Real-Time Spectrogram + Transient Overlay

**Objective:** Scrolling spectrogram with heat-mapped transient detection

**Implementation:**
1. Visualization data pipeline
   - juce::AbstractFifo (lock-free ring buffer)
   - Audio thread: Write FFT magnitudes + transient activities
   - GUI thread: Read at 60fps via Timer callback
2. WebGL spectrogram renderer
   - Fragment shader for colormap (magnitude → RGB)
   - Texture scrolling (horizontal or vertical)
   - Logarithmic frequency axis (20Hz-20kHz)
3. Transient heat overlay
   - Blend transient activity as color overlay
   - Red = high transient, Blue = low transient
   - Synchronized with spectrogram position
4. Performance optimization
   - requestAnimationFrame for smooth 60fps
   - GPU texture updates (texSubImage2D)
   - Downsample to 64 bands for JavaScript (reduce data transfer)
5. Reference implementation
   - Based on [Spectro WebGL spectrogram](https://github.com/calebj0seph/spectro/blob/master/docs/making-of.md)

**Test Criteria:**
- ✓ Spectrogram scrolls smoothly at 60fps
- ✓ Drum hits show bright spots on spectrogram (transient detection visible)
- ✓ Transient heat overlay matches actual transient activity (red on attacks)
- ✓ Frequency axis matches expected ranges (kick low, cymbals high)
- ✓ No audio dropouts during visualization (separate thread)
- ✓ UI remains responsive while audio plays

**Git Commit:** "feat(O-SpectralShaper): Phase 3.3 - Real-time spectrogram with transient heat overlay"

---

**Stage 3 Complete When:**
- All 3 phases pass test criteria
- WebView UI loads without errors
- Parameter binding works bidirectionally
- Spectrogram updates in real-time without stuttering
- Drawable curves synchronize with audio processing

---

### Stage 4: Integration & Testing
**Agent:** testing-agent (or manual)
**Duration:** ~30 minutes

**Objectives:**
1. End-to-end validation
2. Performance optimization
3. Edge case testing

**Test Suite:**

**Functional Tests:**
- ✓ Load plugin in multiple DAWs (Logic Pro, Ableton Live, Reaper)
- ✓ All parameters automatable in DAW
- ✓ Preset save/load preserves all settings (including curves)
- ✓ Plugin state persists in project save/load

**Audio Quality Tests:**
- ✓ Null test: Bypass mode produces bit-identical output
- ✓ Impulse response: No pre-ringing or post-ringing artifacts
- ✓ Frequency sweep: No coloration (flat frequency response in bypass)
- ✓ Drum loop: Transient boost/cut audible and clean

**Performance Tests:**
- ✓ CPU usage <50% single core @ 44.1kHz stereo, 512-sample buffer
- ✓ No audio dropouts at minimum buffer size (64 samples)
- ✓ Latency reported correctly to DAW (512 samples + lookahead)
- ✓ Real-time safe: No allocations, locking, or blocking in processBlock()

**Edge Case Tests:**
- ✓ Sample rate changes (44.1 → 96 → 44.1 kHz) without crashes
- ✓ Buffer size changes (64 → 2048 → 64 samples) without artifacts
- ✓ Extreme parameter values (sensitivity = 0%, 100%)
- ✓ Rapid curve drawing (stress test thread synchronization)
- ✓ Mono input (plugin handles gracefully)
- ✓ Silent input (no denormal CPU spikes)

**Git Commit:** "test(O-SpectralShaper): Stage 4 - Integration tests and performance validation"

---

### Stage 5: Polish & Release
**Agent:** Manual (developer review)
**Duration:** ~20 minutes

**Tasks:**
1. Preset creation
   - "Default" (flat curves, 50% sensitivity)
   - "Drum Punch" (boost low-mid attack, cut high sustain)
   - "Snare Crack" (boost 2-5kHz attack)
   - "Kick Thump" (boost 50-100Hz attack, cut 200Hz sustain)
   - "Cymbal Tame" (cut 8-16kHz attack)
2. Documentation
   - User manual (parameter descriptions, workflow examples)
   - Preset descriptions
3. Final polish
   - UI animations (smooth curve drawing)
   - Tooltips on hover
   - Version number in UI (v1.0.0)
4. Installation
   - Build script (build-and-install.sh)
   - Code signing (macOS)
   - Cache clearing (AU, VST3)
5. Final validation
   - Load in DAW, test all presets
   - Verify AU/VST3 both work
   - Check CPU meter in DAW (confirm <50% usage)

**Git Commit:** "release(O-SpectralShaper): v1.0.0 - Initial release"

---

## Risk Mitigation

### High-Risk Items

**1. FFT Latency (11.6ms @ 44.1kHz)**
- **Mitigation:** Accept trade-off (competitors have similar latency)
- **Fallback:** Implement 256-sample FFT at high sample rates (96kHz+)
- **Validation:** Measure latency with DAW compensation enabled

**2. CPU Usage (Estimated ~30% single core)**
- **Mitigation:** SIMD optimization for band magnitude calculations
- **Fallback:** "Quality" mode selector (High/Balanced/Low CPU)
- **Validation:** Profile with Instruments (macOS) during Stage 4

**3. WebGL Spectrogram Performance**
- **Mitigation:** Use GPU texture scrolling, downsample to 64 bands
- **Fallback:** HTML5 Canvas with 30fps update rate (not 60fps)
- **Validation:** Test on low-end hardware (MacBook Air 2019)

### Medium-Risk Items

**4. Curve Synchronization (JavaScript → C++)**
- **Mitigation:** Double-buffering with atomic flag
- **Validation:** Rapid curve drawing stress test (Stage 4)

**5. Phase Coherence**
- **Mitigation:** Magnitude-only processing, preserve phase
- **Validation:** Mono sum test (L+R should not cancel)

---

## Implementation Notes

### JUCE Module Dependencies

**CMakeLists.txt:**
```cmake
target_link_libraries(O-SpectralShaper
    PRIVATE
        juce::juce_audio_processors  # Core plugin
        juce::juce_dsp               # FFT, windowing, dry/wet mixer
        juce::juce_gui_extra         # WebBrowserComponent
)

juce_generate_juce_header(O-SpectralShaper)  # CRITICAL for JUCE 8

target_compile_definitions(O-SpectralShaper
    PUBLIC
        JUCE_WEB_BROWSER=1           # Enable WebView
        JUCE_USE_CURL=0              # No external network
        JUCE_VST3_CAN_REPLACE_VST2=0
)
```

### Critical Patterns (from juce8-critical-patterns.md)

1. **juce_generate_juce_header()** - MUST be called after target_link_libraries
2. **NEEDS_WEB_BROWSER TRUE** - Required for VST3 WebView support
3. **WebSliderParameterAttachment** - JUCE 8 requires 3 parameters (parameter, relay, undoManager=nullptr)
4. **ES6 Module Loading** - index.html must use `<script type="module">`
5. **WebView Member Initialization** - Use std::unique_ptr, correct order (relays → webView → attachments)

### Thread Safety Checklist

- ✓ No locking in processBlock()
- ✓ APVTS for parameter changes (thread-safe)
- ✓ juce::AbstractFifo for visualization data (lock-free)
- ✓ std::atomic for curve buffer swapping
- ✓ juce::SmoothedValue for parameter smoothing (prevents zipper noise)

### Performance Optimization Opportunities

**Phase 2.2 (DSP):**
- SIMD for band magnitude: `juce::dsp::SIMDRegister<float>`
- FloatVectorOperations for overlap-add

**Phase 3.3 (Visualization):**
- WebGL fragment shader for colormap
- Downsample FFT bins (257 → 64 bands) before sending to JavaScript
- requestAnimationFrame for 60fps synchronization

---

## Success Criteria

**O-SpectralShaper v1.0.0 is complete when:**

1. ✓ All 6 stages pass test criteria
2. ✓ Plugin loads in DAW without errors
3. ✓ Audio processing is artifact-free (no clicks, phase issues)
4. ✓ Transient detection is audible and responsive
5. ✓ Drawable curves work in both freehand and node modes
6. ✓ Spectrogram displays real-time audio with transient overlay
7. ✓ CPU usage <50% single core @ 44.1kHz stereo
8. ✓ Latency ~11.6ms (acceptable for mixing context)
9. ✓ WebView UI loads without errors
10. ✓ All parameters automatable in DAW
11. ✓ Presets save/load correctly
12. ✓ Code committed to git with descriptive messages

---

## Next Steps

**After Stage 0 completion:**

1. **Run `/implement O-SpectralShaper`**
   - Invokes plugin-workflow skill
   - Orchestrates foundation-shell-agent → dsp-agent → gui-agent
   - Each agent executes phases in sequence

2. **Developer Pauses:**
   - After Stage 1: Verify plugin builds and loads
   - After Stage 2 Phase 2.1: Null test (perfect reconstruction)
   - After Stage 2 Phase 2.2: Validate transient detection
   - After Stage 2 Phase 2.3: Test attack/sustain shaping
   - After Stage 3 Phase 3.1: Verify WebView loads
   - After Stage 3 Phase 3.2: Test drawable curves
   - After Stage 3 Phase 3.3: Validate spectrogram
   - After Stage 4: Full integration test suite

3. **Final Review:**
   - Load in DAW, play drum loop
   - Draw attack curve → Hear transient boost
   - Check spectrogram → See transient heat overlay
   - Verify CPU usage <50%
   - Save preset, reload project → Curves persist

**Estimated Total Time:** ~3 hours (Research: 30min, Foundation: 20min, DSP: 60min, GUI: 90min, Testing: 30min, Polish: 20min)

---

**End of Roadmap**

*O-SpectralShaper represents a challenging but achievable plugin implementation. The phased approach ensures each component is validated before moving forward, minimizing rework and integration issues.*
