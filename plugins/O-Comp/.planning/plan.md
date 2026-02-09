# OuariconComp - Implementation Plan

**Date:** 2026-01-11
**Complexity Score:** 5.0 (Capped - Moderate Complexity)
**Strategy:** Single-pass implementation with careful testing

---

## Complexity Factors

**Calculation breakdown:**

- **Parameters:** 7 parameters (7/5 = 1.4 points, capped at 2.0) = **1.4**
  - THRESHOLD, RATIO, ATTACK, RELEASE, KNEE, OUTPUT_GAIN, AUTO_GAIN

- **Algorithms:** 4 DSP components = **4**
  1. Compressor Engine (custom implementation)
  2. Variable Soft-Knee Curve (polynomial interpolation)
  3. Envelope Follower (peak detection with attack/release)
  4. Makeup Gain (auto-gain calculation)

- **Features:** 0 points
  - No feedback loops, FFT, multiband, modulation systems, or MIDI control

- **Total:** 1.4 + 4 + 0 = **5.4 (capped at 5.0)**

**Note:** Score reaches cap, but all algorithms are standard DSP (well-documented, straightforward). No exotic features. Treat as **moderate complexity** (~2.8-2.9 effective) rather than high complexity.

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ← Next
- Stage 1: Foundation
- Stage 2: Shell
- Stage 3: DSP (single-pass)
- Stage 3: GUI (single-pass)
- Stage 3: Validation

---

## Single-Pass Implementation (Moderate Complexity)

### Implementation Flow

- **Stage 1: Foundation** - Project structure, CMakeLists.txt, build system
- **Stage 2: Shell** - APVTS parameters (7 parameters)
- **Stage 3: DSP** - Custom compressor implementation (single pass)
- **Stage 3: GUI** - WebView UI with 7 controls (single pass)
- **Stage 3: Validation** - Presets, pluginval, changelog

---

## Implementation Notes

### DSP Approach

**Core algorithm: Custom feed-forward compressor**

1. **Level detection:**
   - Peak envelope follower (preferred for transparency)
   - Stereo-linked (max of L/R channels prevents image shift)
   - Attack/release ballistics (exponential smoothing)

2. **Gain calculation:**
   - Soft-knee curve (quadratic interpolation, 0-20 dB variable knee)
   - Threshold check with knee region handling
   - Ratio-based gain reduction

3. **Envelope smoothing:**
   - Attack coefficient: Fast response to increasing signals
   - Release coefficient: Smooth decay when signals decrease
   - Formula: `coeff = 1 - exp(-1 / (timeMs * sampleRate / 1000))`

4. **Makeup gain:**
   - Manual: OUTPUT_GAIN parameter (-12 to +24 dB)
   - Automatic: `makeupGain_dB = -threshold * (1 - 1/ratio)` (when AUTO_GAIN enabled)

**JUCE helpers:**
- `juce::Decibels::gainToDecibels()` / `decibelsToGain()` - Safe dB conversions
- `juce::dsp::ProcessSpec` - Sample rate configuration in `prepareToPlay()`
- `juce::dsp::AudioBlock` - Modern buffer processing (JUCE 8 pattern)
- `juce::dsp::ScopedNoDenormals` - Denormal protection in `processBlock()`

**Why custom implementation:**
- `juce::dsp::Compressor` does NOT support knee parameter (required by spec)
- Custom implementation provides full control over soft-knee curve
- Compressor algorithm is well-documented and straightforward

**Module dependency:**
- `juce_dsp` for ProcessSpec, AudioBlock, Decibels helpers
- Add to CMakeLists.txt: `target_link_libraries(... juce::juce_dsp)`

---

### GUI Approach

**WebView-based UI (Ouaricon Audio Naturalist aesthetic):**

- **Layout:** 7 controls on aged paper background (#F5E6D3)
  - Row 1: THRESHOLD, RATIO, KNEE (compression settings)
  - Row 2: ATTACK, RELEASE, OUTPUT (envelope + gain)
  - Row 3: AUTO-GAIN toggle button (centered)

- **Botanical imagery:** Anatomy or skeleton imagery (right side, 35% opacity)
  - Rationale: Structure, control, force - essence of compression

- **Controls:**
  - 6 rotary knobs (60px diameter, botanical seed cross-section design)
  - 1 toggle button (AUTO-GAIN, styled button with on/off states)
  - Garamond serif typography for labels and values

- **Parameter binding:**
  - `getSliderState()` for 6 continuous parameters
  - `getToggleState()` for AUTO_GAIN boolean parameter
  - Two-way binding: UI → DSP and DAW automation → UI

- **Optional enhancement:** Gain reduction meter (vertical bar, left side, earth-tone gradient)
  - Shows real-time dB of gain reduction
  - Requires C++ → JS updates via custom native function

---

### Key Considerations

**Thread safety:**
- All parameter reads in audio thread: `getRawParameterValue()->load()` (atomic)
- Coefficient updates in audio thread (no allocations, just math)
- APVTS handles thread-safe parameter updates from UI

**Performance:**
- Estimated CPU: ~5-10% single core at 48 kHz
- Per-sample processing (no FFT, no convolution)
- Suitable for real-time, multiple instances

**Latency:**
- Zero latency (feed-forward design, no lookahead)
- `getLatencySamples()` returns 0

**Denormal protection:**
- `juce::ScopedNoDenormals` in `processBlock()` (JUCE 8 pattern)
- Envelope state uses exponential smoothing (no denormal risk)

**Sample rate handling:**
- Attack/release coefficients depend on sample rate
- Recalculate in `prepareToPlay()` when sample rate changes
- Supports 44.1 kHz to 192 kHz

---

### Known Challenges

**1. Soft-knee curve tuning:**
- **Challenge:** Knee curve shape affects perceived transparency
- **Mitigation:** Start with hard knee (0 dB), test threshold/ratio/attack/release first
- **Reference:** FabFilter Pro-C 2 knee behavior, professional plugins
- **Fallback:** If knee proves difficult, ship v1.0 with hard knee, add soft-knee in v1.1

**2. Auto-gain calculation accuracy:**
- **Challenge:** Formula `makeupGain_dB = -threshold * (1 - 1/ratio)` is approximation
- **Mitigation:** Provide OUTPUT_GAIN parameter for user fine-tuning
- **Testing:** Test with various threshold/ratio combinations, verify gain matches expectation

**3. Stereo-linked detection implementation:**
- **Challenge:** Must take max of L/R channels before gain calculation
- **Mitigation:** Clear code structure, verify no image shift in stereo test signals
- **Testing:** Use stereo test tones with L/R level differences

**4. Attack/release coefficient edge cases:**
- **Challenge:** Very fast attack (0.1 ms) or very slow release (1000 ms) may cause artifacts
- **Mitigation:** Clamp coefficient values, test extreme parameter settings
- **Reference:** Use exponential formula from professional DSP resources

---

## Testing Strategy

### Unit Testing (Manual)

1. **Hard knee compression:**
   - Set KNEE = 0 dB
   - Test threshold crossing (gain reduction starts exactly at threshold)
   - Test ratio accuracy (4:1 should reduce by 3 dB for every 4 dB above threshold)

2. **Soft-knee compression:**
   - Set KNEE = 6 dB, then 12 dB, then 20 dB
   - Verify smooth transition around threshold
   - No clicks or discontinuities when sweeping input level

3. **Attack/release ballistics:**
   - Fast attack (0.1 ms): Compresses transients immediately
   - Slow attack (100 ms): Lets transients through, compresses body
   - Fast release (10 ms): Can cause pumping on sustained material
   - Slow release (1000 ms): Smooth, musical release

4. **Auto-gain accuracy:**
   - Enable AUTO_GAIN
   - Set threshold = -20 dB, ratio = 4:1
   - Expected auto-gain: ~15 dB (verify output level roughly matches input)

5. **Stereo-linked behavior:**
   - Feed stereo test signal with L louder than R
   - Verify both channels receive same gain reduction (no image shift)

### Integration Testing

1. **DAW compatibility:**
   - Load in Logic Pro, Ableton Live, Reaper
   - Verify parameter automation works
   - Verify preset loading/saving

2. **Edge cases:**
   - Ratio = 1:1 (no compression, unity gain)
   - Ratio = 20:1 (heavy limiting)
   - Threshold = -60 dB (compresses everything)
   - Threshold = 0 dB (compresses nothing until peaks)

3. **Performance:**
   - Load 10+ instances on single track
   - Verify CPU usage stays reasonable (<50% single core)
   - No audio dropouts or glitches

---

## References

**Contract files:**
- Creative brief: `plugins/OuariconComp/.ideas/creative-brief.md`
- Parameter spec: Creative brief (parameters section)
- DSP architecture: `plugins/OuariconComp/.ideas/architecture.md`

**Similar plugins for reference:**
- **OuariconTremolo:** Simple modulation effect, WebView UI, similar complexity
  - Reference for: WebView parameter binding, knob interaction, UI layout
- **OuariconSaturationModeling:** DSP-focused effect, custom algorithm
  - Reference for: Custom DSP implementation, JUCE helpers usage
- **OuariconMarimba:** Instrument with multiple parameters
  - Reference for: APVTS setup with 7+ parameters

**JUCE documentation:**
- `juce::dsp::Compressor`: [docs.juce.com](https://docs.juce.com/master/classdsp_1_1Compressor.html) (reference, but missing knee)
- `juce::Decibels`: dB ↔ linear conversion
- `juce::dsp::ProcessSpec`: Modern DSP configuration
- Critical patterns: `/troubleshooting/patterns/juce8-critical-patterns.md`

**Professional plugin references:**
- FabFilter Pro-C 2 (transparent compression standard)
- Waves Renaissance Compressor (parameter ranges)
- UAD 1176 (attack/release time references)

---

## Duration Estimates

- **Stage 1: Foundation** - 30 minutes (CMakeLists.txt, project structure)
- **Stage 2: Shell** - 45 minutes (7 APVTS parameters)
- **Stage 3: DSP** - 3-4 hours (custom compressor implementation, testing)
  - Hard knee first: 1.5 hours
  - Soft-knee curve: 1-1.5 hours
  - Auto-gain + makeup gain: 30 minutes
  - Testing and tuning: 30-45 minutes
- **Stage 3: GUI** - 2-3 hours (WebView UI, parameter binding, layout)
- **Stage 3: Validation** - 1 hour (presets, pluginval, changelog)

**Total estimated time:** 7-9 hours

---

## Success Criteria

**DSP:**
- [ ] Compressor responds to threshold, ratio, attack, release parameters
- [ ] Soft-knee curve provides smooth transition (0-20 dB variable)
- [ ] Auto-gain calculation works correctly (roughly compensates for gain reduction)
- [ ] Stereo-linked compression prevents image shift
- [ ] Zero latency, suitable for real-time use
- [ ] CPU usage < 15% single core for single instance

**GUI:**
- [ ] All 7 parameters visible and functional
- [ ] Knobs respond to mouse drag (relative drag, not absolute)
- [ ] DAW automation updates UI controls in real-time
- [ ] Preset changes update all controls correctly
- [ ] WebView loads without errors
- [ ] Layout matches Ouaricon Audio Naturalist aesthetic

**Validation:**
- [ ] Passes pluginval (AU and VST3 formats)
- [ ] 5+ factory presets included
- [ ] Changelog documents v1.0.0 release
- [ ] No crashes, no audio glitches, no parameter jumps
