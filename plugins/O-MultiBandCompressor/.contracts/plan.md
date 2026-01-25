# O-MultiBandCompressor - Implementation Plan

**Date:** 2026-01-25
**Complexity Score:** 5.0 (Complex - Maximum)
**Strategy:** Phase-based implementation

---

## Complexity Factors

**Calculation breakdown:**

- **Parameters:** 57 parameters (57/5 = 11.4, capped at 2.0) = **2.0**
- **Algorithms:** 10 DSP components = **10**
  - Linkwitz-Riley Crossover Network (cascaded Butterworth filters)
  - Per-Band Feed-Forward Compressor (×4 bands)
  - Envelope Detector (Peak + RMS blend)
  - Soft Knee Gain Computer
  - Per-Band Sidechain Filter
  - Mid/Side Encoding/Decoding
  - Auto-Makeup Gain Calculator
  - Global Dry/Wet Mixer
  - FFT Spectrum Analyzer
  - Gain Reduction Metering (×4 bands)
- **Features:** 2 points
  - FFT/frequency domain processing (+1)
  - Multiband processing with band summation (+1)
- **Total:** 2.0 + 10 + 2 = **14.0** → capped at **5.0**

**Classification:** COMPLEX (Maximum complexity score)

---

## Stages

- Stage 0: Research ✓ (Complete)
- Stage 1: Planning ← Next
- Stage 1: Foundation
- Stage 2: Shell
- Stage 3: DSP - 3 phases
- Stage 4: GUI - 3 phases
- Stage 5: Validation

---

## Complex Implementation (Score = 5.0)

### Stage 3: DSP Phases

#### Phase 4.1: Single-Band Compressor Foundation

**Goal:** Implement core compression engine with all features on single band (no crossover yet)

**Components:**
- Basic stereo audio path (input → processing → output)
- Feed-forward compressor with soft knee
- Peak detector
- RMS detector with circular buffer
- Peak/RMS blend
- Envelope smoother (attack/release ballistics)
- Soft knee gain computer (threshold/ratio/knee)
- Manual makeup gain
- Input/output gain stages
- Bypass logic

**Parameters implemented:**
- INPUT_GAIN
- OUTPUT_GAIN
- THRESHOLD (single band)
- RATIO (single band)
- ATTACK (single band)
- RELEASE (single band)
- KNEE (single band)
- MAKEUP (single band)
- PEAK_RMS_BLEND (single band)
- BYPASS (single band)

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through cleanly when bypassed
- [ ] Compression audibly reduces dynamic range
- [ ] Threshold parameter triggers compression at correct level
- [ ] Ratio parameter scales gain reduction correctly
- [ ] Attack/release parameters control compression speed
- [ ] Soft knee smooths compression onset (compare 0dB vs 12dB knee)
- [ ] Peak/RMS blend changes compression character (test with drums)
- [ ] Makeup gain compensates for gain reduction
- [ ] No clicks, pops, or discontinuities during parameter changes
- [ ] GR metering displays correct values (0 to -24 dB range)

**Success metric:** Single-band compressor with all features working correctly

---

#### Phase 4.2: Linkwitz-Riley Crossover Network + Multiband

**Goal:** Add 4-band crossover and replicate compressor across all bands

**Components:**
- Linkwitz-Riley 4th order crossover (3 crossover points)
- Cascaded 2nd order Butterworth filters (12 IIR filters per channel)
- Crossover coefficient calculation with smooth interpolation
- Band summation (recombine all 4 bands)
- Per-band compressor instances (4 compressors total)
- Per-band parameter management (4 sets of compressor params)
- Solo/bypass routing per band

**Parameters added:**
- XOVER1 (Low/Low-Mid crossover)
- XOVER2 (Low-Mid/High-Mid crossover)
- XOVER3 (High-Mid/High crossover)
- All compressor parameters × 4 bands (THRESHOLD, RATIO, ATTACK, RELEASE, KNEE, MAKEUP, PEAK_RMS, BYPASS)
- SOLO × 4 bands

**Test Criteria:**
- [ ] White noise input produces flat spectrum output (crossover summing test)
- [ ] Each crossover point splits spectrum correctly (verify with pink noise + FFT)
- [ ] Crossover frequency parameters adjust split points smoothly
- [ ] No clicks when changing crossover frequencies
- [ ] Each band compresses independently (test with band-limited signals)
- [ ] Solo mode isolates single band (all others muted)
- [ ] Bypass mode passes band through unprocessed
- [ ] All 4 bands sum to unity gain when no compression applied
- [ ] Low band compresses bass without affecting highs
- [ ] High band compresses treble without affecting lows
- [ ] Phase alignment verified (no comb filtering artifacts)

**Success metric:** 4-band multiband compressor with independent per-band control

---

#### Phase 4.3: Advanced Features (Sidechain + M/S + Auto-Makeup + Dry/Wet)

**Goal:** Implement remaining DSP features for professional multiband compressor

**Components:**
- Per-band sidechain filtering (HPF/LPF on detector input)
- Sidechain listen mode (monitor filtered detector signal)
- Mid/Side encoding/decoding
- M/S processing modes (Off/Mid/Side/Both)
- Auto-makeup gain calculation (per band)
- Global dry/wet mixer with latency compensation

**Parameters added:**
- SC_HPF × 4 bands (sidechain high-pass filter)
- SC_LPF × 4 bands (sidechain low-pass filter)
- SC_LISTEN × 4 bands (monitor sidechain signal)
- MS_MODE (Off/Mid/Side/Both)
- AUTO_MAKEUP (global on/off)
- MIX (dry/wet blend)

**Test Criteria:**
- [ ] Sidechain HPF reduces low-frequency compression trigger (test with bass + click)
- [ ] Sidechain LPF reduces high-frequency compression trigger (de-essing test)
- [ ] SC Listen mode outputs filtered detector signal correctly
- [ ] M/S encoding preserves level (unity gain test with mono and stereo signals)
- [ ] Mid-only mode compresses center without affecting stereo width
- [ ] Side-only mode compresses stereo width without affecting center
- [ ] Both mode provides independent mid/side compression (8 compressors active)
- [ ] Auto-makeup compensates for gain reduction within 500ms
- [ ] Auto-makeup doesn't cause pumping artifacts
- [ ] Dry/wet mix blends processed signal smoothly (0-100%)
- [ ] Dry/wet mixer compensates for processing latency (dry/wet phase aligned)

**Success metric:** Full-featured multiband compressor with sidechain, M/S, auto-makeup, and parallel compression

---

### Stage 4: GUI Phases

#### Phase 5.1: Layout and Basic Controls

**Goal:** WebView integration with spectrum analyzer visualization and crossover controls

**Components:**
- WebView setup in PluginEditor
- HTML/CSS layout matching Botanical/Ouaricon aesthetic
- Spectrum analyzer canvas (FFT visualization)
- Band overlay regions (colored frequency zones)
- Draggable crossover handles on spectrum
- Input/output meters (stereo peak + RMS)
- Global controls (Input Gain, Output Gain, Mix, Auto-Makeup, M/S Mode)
- Per-band section headers (Low, Low-Mid, High-Mid, High)

**Test Criteria:**
- [ ] WebView window opens with correct size (~900×600px)
- [ ] Botanical/Ouaricon styling renders correctly (paper texture, warm colors)
- [ ] Spectrum analyzer displays frequency content (20Hz-20kHz, -80dB to 0dB)
- [ ] Band overlay regions show 4 frequency zones with crossover points
- [ ] Crossover handles are draggable and snap to valid frequencies
- [ ] Input/output meters display peak and RMS levels
- [ ] Global controls visible and styled correctly
- [ ] Layout is responsive and clean

---

#### Phase 5.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI ↔ DSP) for all 57 parameters

**Components:**
- JavaScript → C++ relay calls for all parameters
- C++ → JavaScript parameter updates (host automation)
- Per-band compressor controls (knobs for Threshold, Ratio, Attack, Release, Knee, Makeup, Peak/RMS)
- Per-band buttons (Solo, Bypass, SC Listen)
- Sidechain filter controls (HPF/LPF per band)
- Value formatting and display (dB, ms, ratio, %)
- Real-time parameter updates during playback

**Test Criteria:**
- [ ] All knobs rotate smoothly with mouse drag (relative drag, not absolute)
- [ ] Knob values update DSP parameters correctly
- [ ] Host automation updates knobs in real-time
- [ ] Preset changes update all 57 UI elements
- [ ] Parameter values display with correct formatting (e.g., "-12.0 dB", "4:1", "50 ms")
- [ ] Buttons toggle states correctly (Solo, Bypass, SC Listen, Auto-Makeup)
- [ ] Crossover handles update DSP crossover frequencies
- [ ] M/S mode selector changes processing mode correctly
- [ ] No lag or visual glitches during parameter changes
- [ ] WebSliderParameterAttachment uses 3-parameter constructor (parameter, relay, nullptr)

---

#### Phase 5.3: Advanced UI Elements (GR Meters + Spectrum Features)

**Goal:** Real-time visualization of compression activity and frequency content

**Components:**
- Per-band gain reduction meters (vertical bars, 0 to -24 dB)
- GR meter ballistics (peak-hold with slow decay)
- FFT processing on separate thread
- Lock-free communication (audio → FFT → UI)
- Spectrum temporal smoothing (70% old + 30% new)
- Spectrum update throttling (30-60 fps)
- Crossover point indicators on spectrum
- Input/output meter animations with ballistic motion

**Test Criteria:**
- [ ] GR meters display compression activity per band
- [ ] GR meters peak-hold and decay smoothly (analog VU meter ballistics)
- [ ] Spectrum analyzer updates in real-time (30-60 fps)
- [ ] Spectrum displays logarithmic frequency scale (20Hz-20kHz)
- [ ] Spectrum shows dB magnitude (-80dB to 0dB range)
- [ ] Crossover points visible as vertical lines on spectrum
- [ ] Band regions color-coded and match crossover positions
- [ ] Spectrum smoothing prevents jittery display
- [ ] No UI thread starvation (smooth animation even with audio processing)
- [ ] FFT thread communication doesn't cause audio glitches
- [ ] Input/output meters animate with ballistic motion (fast attack, slow decay)

---

### Implementation Flow

- **Stage 0:** Research ✓ (Complete - Architecture and Plan documented)
- **Stage 1:** Planning ← Next
- **Stage 1:** Foundation - Project structure, CMakeLists.txt, PluginProcessor skeleton
- **Stage 2:** Shell - APVTS parameters (all 57 parameters defined)
- **Stage 3:** DSP - 3 phases
  - **Phase 4.1:** Single-Band Compressor Foundation (core compression engine)
  - **Phase 4.2:** Linkwitz-Riley Crossover + Multiband (4-band architecture)
  - **Phase 4.3:** Advanced Features (Sidechain + M/S + Auto-Makeup + Dry/Wet)
- **Stage 4:** GUI - 3 phases
  - **Phase 5.1:** Layout and Basic Controls (WebView + Spectrum + Crossover Handles)
  - **Phase 5.2:** Parameter Binding (57 parameters, two-way communication)
  - **Phase 5.3:** Advanced UI (GR Meters + FFT Visualization)
- **Stage 5:** Validation - Presets, pluginval, changelog, final testing

---

## Implementation Notes

### Thread Safety

**Audio thread:**
- All DSP processing (crossover, compression, M/S, mixing)
- Parameter reads via `APVTS::getRawParameterValue()->load()` (atomic)
- Crossover coefficient recalculation (in audio thread, no allocations)
- GR meter value updates (`std::atomic<float>` writes)
- FFT input buffer filling (copy to circular buffer)

**FFT Processing thread:**
- FFT computation (separate thread to avoid audio glitches)
- Spectrum smoothing and magnitude calculation
- Throttled UI updates (30-60 fps via timer)

**Message thread (UI):**
- Parameter updates from UI via APVTS (atomic writes)
- Spectrum display rendering (Canvas in WebView)
- GR meter visual updates (reads from atomic values)
- Crossover handle dragging (updates XOVER parameters)
- Preset loading/saving

**Communication mechanisms:**
- APVTS parameters: Atomic reads (audio) / atomic writes (UI)
- GR meters: `std::array<std::atomic<float>, 4>` (audio writes, UI reads)
- FFT spectrum: Lock-free circular buffer or `AbstractFifo` (audio writes, FFT thread reads)
- No mutexes in audio thread

---

### Performance

**Estimated CPU usage (per component, stereo @ 48kHz):**
- Linkwitz-Riley Crossover (24 IIR filters): ~12% CPU
- Compressors (4 bands × 2 channels): ~8% CPU
- Peak/RMS Detectors (4 bands × 2 detectors): ~3% CPU
- Sidechain Filters (8 filters): ~2% CPU
- M/S Encoding/Decoding: <1% CPU
- Dry/Wet Mixer: <1% CPU
- FFT Analysis (separate thread, not audio thread): ~5% CPU
- **Total estimated (Off mode):** ~25% single core
- **Total estimated (Both mode):** ~35% single core (8 compressors)

**Optimization opportunities:**
- SIMD for crossover filtering (JUCE supports SSE/NEON)
- Vectorize gain reduction application (process 4 samples at once)
- Pre-calculate logarithms for threshold/ratio conversions
- Use lookup tables for soft knee quadratic formula
- Batch coefficient updates (only recalculate when parameters change)

**Performance targets:**
- <30% CPU at 48kHz stereo (Off mode)
- <50% CPU at 48kHz stereo (Both mode)
- <50% CPU at 96kHz stereo (Off mode)

---

### Latency

**Total processing latency:**
- Crossover IIR filters: ~2 samples (group delay)
- Envelope smoothing lookahead: ~480 samples @ 48kHz (10ms attack time)
- **Total:** ~10-12ms (depends on minimum attack time across all bands)

**Host compensation:**
```cpp
int getLatencySamples() const override {
    int filterLatency = 2; // IIR crossover group delay
    int envelopeLatency = static_cast<int>(0.01f * sampleRate); // 10ms minimum attack
    return filterLatency + envelopeLatency;
}
```

**Dry/wet mixer:** Use `juce::dsp::DryWetMixer` for automatic dry signal delay compensation

---

### Denormal Protection

- Use `juce::ScopedNoDenormals` in `processBlock()`
- All JUCE DSP components handle denormals internally (IIR::Filter, DryWetMixer, FFT)
- Custom code protections:
  - Epsilon addition before log10(): `log10(x + 1e-10f)` prevents -inf
  - RMS circular buffer zero-initialized to prevent denormals in initial window
  - Gain reduction clamping: Ensure GR never produces denormal gains (<1e-30)
  - Soft knee formula uses clamped input values

---

### Known Challenges

**1. Linkwitz-Riley coefficient smoothing:**
- Changing crossover frequencies recalculates IIR coefficients
- Abrupt coefficient changes cause clicks
- **Solution:** Implement 30ms linear interpolation between old/new coefficients
- **Reference:** Check JUCE forum thread on Linkwitz-Riley implementation

**2. Crossover phase alignment validation:**
- Complementary LP/HP must sum to unity gain (flat magnitude response)
- Phase errors cause comb filtering artifacts
- **Solution:** Unit test with white noise input, verify flat spectrum output with FFT
- **Validation:** Plot magnitude response in MATLAB/Python to confirm

**3. Peak/RMS circular buffer management:**
- RMS requires sliding window (10-50ms) of squared samples
- Circular buffer must be thread-safe (write from audio thread, no reads from other threads)
- **Solution:** Use `std::vector` sized in `prepareToPlay()`, wrap index with modulo
- **Reference:** See similar implementation in GainKnob reference plugin

**4. FFT thread communication:**
- Audio thread fills FFT buffer, FFT thread processes, UI thread renders
- Lock-free communication required (no mutexes in audio thread)
- **Solution:** Use `juce::AbstractFifo` for audio → FFT, `std::atomic` flags for FFT → UI
- **Reference:** JUCE FFT tutorial demonstrates lock-free approach

**5. M/S mode switching clicks:**
- Switching between Off/Mid/Side/Both can cause level jumps
- Different number of active compressors (4 vs 8) affects processing
- **Solution:** Implement 30ms crossfade when mode changes, or use zero-crossing detection
- **Fallback:** Document as limitation if crossfade proves complex

**6. Soft knee boundary conditions:**
- Quadratic formula at threshold ± knee/2 boundaries can overshoot
- Edge case: knee width > 2 × threshold (threshold below knee start)
- **Solution:** Clamp knee width to prevent negative threshold boundaries
- **Validation:** Unit test with extreme parameter values (threshold -60dB, knee 24dB)

**7. WebView spectrum rendering performance:**
- 60fps Canvas updates can stress UI thread
- FFT data is 1024 bins (half of 2048 FFT size)
- **Solution:** Throttle updates to 30fps if 60fps causes frame drops
- **Optimization:** Pre-render spectrum to offscreen canvas, blit to visible canvas

**8. Auto-makeup pumping artifacts:**
- Fast-changing gain reduction can cause auto-makeup to pump
- Need slow ballistics (500ms) to average GR over time
- **Solution:** Use longer smoothing window (100ms history buffer)
- **Tuning:** Test with dynamic material (drums), adjust smoothing coefficients

---

## References

**Contract files:**
- Creative brief: `plugins/O-MultiBandCompressor/.ideas/creative-brief.md`
- DSP architecture: `plugins/O-MultiBandCompressor/.contracts/architecture.md`
- Implementation plan: `plugins/O-MultiBandCompressor/.contracts/plan.md`

**Similar plugins for reference:**
- **O-SimpleReverb** - WebView integration, spectrum visualization
- **O-Tremolo** - Modulation, LFO implementation
- **GainKnob** - WebView parameter binding reference implementation
- **TapeAge** - Dry/wet mixing, ballistics filter usage

**JUCE critical patterns:**
- `troubleshooting/patterns/juce8-critical-patterns.md` - Non-negotiable JUCE 8 patterns

**External resources:**
- JUCE FFT Tutorial: https://juce.com/tutorials/tutorial_spectrum_analyser/
- AES Paper: "Digital Dynamic Range Compressor Design" (Giannoulis et al., 2012)
- Linkwitz-Riley Primer: Rane Note 160
- JUCE Forum: Linkwitz-Riley 4th order coefficients discussion

---

**Plan created:** 2026-01-25
**Ready for Stage 1 (Planning):** Yes
**Estimated implementation time:**
- Stage 1 (Foundation + Shell): 2-3 hours
- Stage 3 (DSP - 3 phases): 8-12 hours
- Stage 4 (GUI - 3 phases): 6-10 hours
- Stage 5 (Validation): 2-3 hours
- **Total:** 18-28 hours (highly complex plugin)
