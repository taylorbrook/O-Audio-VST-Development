# O-FreqPulse - Implementation Roadmap

---
**Contract Status:** ROADMAP COMPLETE
**Generated:** 2026-02-03
**Complexity Score:** C4 (Complex - FFT/Spectral Processing + Sequencing)
**Estimated Stages:** 4 (Foundation → DSP → GUI → Polish)
---

## Complexity Assessment

### Scoring Matrix

| Factor | Score | Weight | Weighted |
|--------|-------|--------|----------|
| **DSP Complexity** | 5/5 | 30% | 1.50 |
| **Parameter Count** | 4/5 | 20% | 0.80 |
| **UI Complexity** | 4/5 | 20% | 0.80 |
| **Algorithm Novelty** | 4/5 | 15% | 0.60 |
| **Integration Points** | 3/5 | 15% | 0.45 |
| **Total** | | | **4.15** |

**Final Score: C4 (Complex)**

### Complexity Justification

**DSP Complexity (5/5):**
- STFT overlap-add processing (non-trivial real-time algorithm)
- FFT forward/inverse transforms in real-time audio thread
- Per-bin magnitude manipulation with phase preservation
- 4-band frequency splitting with configurable crossovers
- Latency reporting and management

**Parameter Count (4/5):**
- ~165 parameters total (5 global + 32 band + 128 step)
- Complex parameter interactions (Euclidean mode overrides manual)
- Per-step gain values (beyond simple on/off)

**UI Complexity (4/5):**
- 2D step grid (4 bands × 32 steps = 128 interactive cells)
- Real-time playhead synchronized to host transport
- Euclidean controls per band with live pattern preview
- Band frequency visualization

**Algorithm Novelty (4/5):**
- Euclidean rhythm generation per band (unique feature)
- Tempo-synced spectral gating (uncommon combination)
- Not a standard "cookbook" algorithm

**Integration Points (3/5):**
- Host tempo sync (AudioPlayHead)
- WebView JavaScript bridge
- Standard APVTS parameter binding

---

## Implementation Stages

### Stage 1: Foundation + Shell
**Goal:** Build system, APVTS parameters, plugin scaffolding

**Tasks:**
1. Create CMakeLists.txt with juce_dsp, juce_gui_extra
2. Define all 165 parameters in APVTS
3. Implement basic PluginProcessor/Editor shell
4. Configure VST3/AU build targets

**Deliverables:**
- [ ] Plugin loads in DAW
- [ ] All parameters visible in DAW automation
- [ ] No DSP processing (passthrough)

**Risk:** LOW - Standard JUCE boilerplate

---

### Stage 2: DSP Implementation
**Goal:** Complete STFT spectral gating engine

**Tasks:**
1. **FFT Infrastructure**
   - Implement STFT input/output buffers (2048 samples, 75% overlap)
   - Setup juce::dsp::FFT and WindowingFunction
   - Implement overlap-add reconstruction

2. **Band Processing**
   - Create bin-to-band mapping table
   - Implement per-band gain application
   - Preserve phase information during magnitude scaling

3. **Step Sequencer Engine**
   - Tempo sync via AudioPlayHead (BPM, PPQ position)
   - Calculate current step from PPQ
   - Swing timing implementation

4. **Euclidean Generator**
   - Implement Bresenham/bucket-fill algorithm
   - Pattern rotation (offset parameter)
   - Mode switching (Manual ↔ Euclidean)

5. **Smoothing & Mixing**
   - Per-band gain smoothing (SmoothedValue)
   - Dry/wet mixer integration
   - Latency reporting (setLatencySamples)

**Deliverables:**
- [ ] FFT processing produces audible spectral gating
- [ ] Tempo-synced step sequencing works
- [ ] Euclidean patterns generate correctly
- [ ] No clicks/pops during gate transitions
- [ ] Latency reported to DAW

**Risk:** HIGH - FFT artifacts, performance, timing accuracy

**Mitigation:**
- Test with sine sweeps for phase issues
- Profile CPU at 96kHz
- Implement smoothing early

---

### Stage 3: GUI Implementation
**Goal:** WebView-based 2D step grid interface

**Tasks:**
1. **WebView Setup**
   - Create HTML/CSS/JS structure
   - Configure WebBrowserComponent
   - Setup JUCE JavaScript bridge

2. **Step Grid**
   - 4-row × 32-column clickable grid
   - Visual step state (on/off/gain level)
   - Playhead indicator (requestAnimationFrame)
   - Click-to-toggle interaction

3. **Band Controls**
   - Per-band enable/disable toggle
   - Frequency range display
   - Depth slider
   - Euclidean mode toggle

4. **Euclidean Panel**
   - Steps/Pulses/Offset sliders per band
   - Live pattern preview
   - Visual rhythm representation

5. **Global Controls**
   - Mix knob
   - Rate dropdown
   - Swing slider
   - Smoothing slider
   - Step count selector (4/8/16/32)

6. **Parameter Binding**
   - WebSliderParameterAttachment for knobs
   - WebToggleButtonParameterAttachment for buttons
   - Custom step grid state synchronization

**Deliverables:**
- [ ] Full 2D grid renders correctly
- [ ] Playhead tracks host transport
- [ ] All parameters bound and functional
- [ ] Euclidean controls generate patterns
- [ ] Responsive click interaction (<16ms feedback)

**Risk:** MEDIUM - WebView rendering performance, JavaScript bridge reliability

**Mitigation:**
- Batch DOM updates
- Use CSS transforms for playhead
- Follow juce8-critical-patterns.md exactly

---

### Stage 4: Polish & Validation
**Goal:** Testing, optimization, preset creation

**Tasks:**
1. **Performance Optimization**
   - Profile with Xcode Instruments
   - Optimize FFT bin processing (SIMD if needed)
   - Ensure <5% CPU at 44.1kHz stereo

2. **Audio Quality Testing**
   - A/B test with bypassed signal
   - Check for spectral artifacts on sustained tones
   - Verify transient preservation on drums

3. **Preset Creation**
   - Create 8-12 factory presets
   - Categories: Rhythmic, Ambient, Experimental, Polyrhythmic

4. **Validation**
   - pluginval Level 5
   - DAW testing (Logic, Ableton, Reaper)
   - Sample rate testing (44.1, 48, 96 kHz)

**Deliverables:**
- [ ] CPU <5% at 44.1kHz stereo
- [ ] No audible artifacts
- [ ] pluginval Level 5 pass
- [ ] Factory presets installed
- [ ] README/manual documentation

**Risk:** LOW - Standard validation workflow

---

## Risk Summary

| Risk | Severity | Stage | Mitigation |
|------|----------|-------|------------|
| FFT processing artifacts | HIGH | 2 | Proper COLA, smoothing, phase preservation |
| CPU performance | MEDIUM | 2, 4 | SIMD optimization, profile at 96kHz |
| Latency perception | MEDIUM | 2 | Proper DAW reporting, documentation |
| WebView rendering | MEDIUM | 3 | Batch updates, CSS transforms, RAF |
| Step grid sync | LOW | 3 | Atomic parameter reads, requestAnimationFrame |

---

## Dependencies

### JUCE Modules Required
```cmake
juce_audio_processors   # AudioProcessor, APVTS, AudioPlayHead
juce_dsp                # FFT, WindowingFunction, DryWetMixer, SmoothedValue
juce_gui_extra          # WebBrowserComponent
```

### External Dependencies
- None (pure JUCE implementation)

### Internal Modules (Ouaricon)
- WebView boilerplate from O-Detune/O-Tremolo pattern
- No shared DSP modules (custom FFT processing)

---

## Preset Strategy

### Categories
1. **Rhythmic Gates** - Classic trance gate patterns across frequency bands
2. **Ambient Textures** - Slow Euclidean patterns, high smoothing
3. **Polyrhythmic** - Different Euclidean ratios per band (5, 7, 11)
4. **Bass Punch** - Sub band solid, mids/highs chopping
5. **High Energy** - Fast 1/32 highs, slower lows
6. **Experimental** - Extreme settings, unusual patterns

### Suggested Presets (12 total)
1. Classic Sidechain (Sub solid, mids pump)
2. Trance Gate 16th
3. Dubstep Pulse
4. Ambient Shimmer (slow highs)
5. Polyrhythm 5-7-11
6. Bass Foundation
7. Hi-Hat Chop
8. Full Spectrum Gate
9. Euclidean Groove
10. Half-Time Feel
11. Triplet Bounce
12. Init (default starting point)

---

## Success Criteria

### Functional
- [ ] 4 independent frequency bands with configurable crossovers
- [ ] Step sequencer synced to host tempo
- [ ] Euclidean rhythm generation per band
- [ ] Smooth gate transitions (no clicks)
- [ ] Proper latency compensation

### Performance
- [ ] <5% CPU on Apple Silicon at 44.1kHz stereo
- [ ] <50MB memory footprint
- [ ] 60fps UI rendering

### Quality
- [ ] pluginval Level 5 pass
- [ ] No audible artifacts on sustained tones
- [ ] Accurate tempo tracking

### Usability
- [ ] User creates interesting pattern within 30 seconds
- [ ] Visual feedback within 16ms of interaction
- [ ] Clear mode indication (Manual vs Euclidean)

---

## Implementation Order

```
Stage 1: Foundation ─────────────────────────────────> [1-2 days]
    ├── CMake setup
    ├── APVTS parameters (all 165)
    └── Plugin shell

Stage 2: DSP ────────────────────────────────────────> [3-5 days]
    ├── FFT infrastructure
    │   ├── STFT buffers
    │   ├── Window function
    │   └── Overlap-add
    ├── Band processing
    │   ├── Bin mapping
    │   └── Gain application
    ├── Step sequencer
    │   ├── Tempo sync
    │   ├── PPQ calculation
    │   └── Swing
    ├── Euclidean generator
    └── Smoothing + mixing

Stage 3: GUI ────────────────────────────────────────> [2-3 days]
    ├── WebView setup
    ├── Step grid
    │   ├── 2D rendering
    │   ├── Playhead
    │   └── Click handling
    ├── Band controls
    ├── Euclidean panel
    └── Parameter binding

Stage 4: Polish ─────────────────────────────────────> [1-2 days]
    ├── Performance optimization
    ├── Audio quality testing
    ├── Presets
    └── Validation
```

---

**Total Estimated Duration:** 7-12 days (C4 complexity)

---

**Generated:** 2026-02-03
**Status:** ROADMAP COMPLETE - Ready for /implement
