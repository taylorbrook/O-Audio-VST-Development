# O-Detune v1.1.0 Implementation Plan

**Created:** 2026-02-02
**Version Bump:** 1.0.0 → 1.1.0 (MINOR)
**Reason:** Multiple feature completions + UI overhaul

---

## Overview

This improvement addresses 4 issues:
1. UI aesthetic → Ouaricon Naturalist (botanical theme)
2. Disconnected parameters → Full DSP implementation (14 parameters)
3. Random wobble shape → True randomness (non-repeating)
4. Era character → Meaningful sonic differences

---

## Implementation Waves

### Wave 1: DSP Foundation (PluginProcessor.cpp)

**Task 1.1: Implement LFO Shape Selection**
- Read `wobble_shape` parameter in processBlock
- Sine: existing behavior (keep)
- Triangle: `2.0f * std::abs(2.0f * (phase - std::floor(phase + 0.5f))) - 1.0f`
- Random: Sample-and-hold with smoothing (see Algorithm section)

**Task 1.2: Implement True Random LFO**
```cpp
// Member variables for random LFO
float randomCurrentValue = 0.0f;
float randomTargetValue = 0.0f;
float randomSmoothingCoeff = 0.0f;
int randomHoldCounter = 0;
int randomHoldSamples = 0;
juce::Random randomGenerator;

// In processBlock (when shape == Random):
if (randomHoldCounter >= randomHoldSamples) {
    randomTargetValue = randomGenerator.nextFloat() * 2.0f - 1.0f;
    randomHoldSamples = static_cast<int>(currentSampleRate / wobbleRate);
    randomHoldCounter = 0;
}
randomCurrentValue += (randomTargetValue - randomCurrentValue) * randomSmoothingCoeff;
randomHoldCounter++;
```

**Task 1.3: Implement Era Character**
- 60s (Ampex): Rate *= 0.7, depth *= 0.8, add low-pass (2kHz)
- 70s (Teac): Default behavior (no modification)
- 80s (Cassette): Rate *= 1.3, add high-shelf boost, add noise modulation

**Task 1.4: Dynamic Unison Voice Count**
- Read `unison_voices` parameter
- Map choice index to voice count: {0→2, 1→3, 2→4, 3→5, 4→7}
- Adjust voice distribution loop dynamically

**Task 1.5: Implement Unison Distribution**
- Linear: Equal spacing across detune range (existing)
- Exponential: Voices cluster toward center, edges more sparse
- Random: Per-voice random offset from linear

**Task 1.6: Implement Stereo Spread (Unison)**
- Read `unison_spread` parameter
- Calculate per-voice pan position
- Apply pan law (constant power or linear)

**Task 1.7: Implement Character Section**
- Drive: `tanh(input * (1 + drive * 3))` waveshaping
- Color: Blend between low-pass (negative) and high-shelf (positive)
- Age: Mix in subtle noise + filter coefficient drift

**Task 1.8: Implement Advanced Section**
- Pre-delay: Additional delay line before processing
- Feedback: Recirculation with gain limiting
- Randomization: Per-voice variation in detune amount

**Task 1.9: Implement Mono-Safe**
- When enabled: Ensure stereo correlation stays positive
- Could use mid-side processing with side limiting

**Task 1.10: Implement Tempo Sync**
- Read `wobble_sync` parameter
- Get tempo from playhead: `getPlayHead()->getPosition()->getBpm()`
- Calculate rate as note division of tempo

### Wave 2: UI Overhaul (index.html)

**Task 2.1: Apply Ouaricon Naturalist Color Palette**
```css
:root {
    --bg-paper: #F5E6D3;
    --brown-border: #8B7355;
    --brown-frame: #5C4033;
    --brown-text: #3C2F2F;
    --green-light: #8BA870;
    --green-mid: #6B8E4E;
    --green-dark: #3C5C1A;
    --knob-segment-1: #F5DEB3;
    --knob-segment-2: #E8D5B7;
    --knob-core: #FFF8DC;
}
```

**Task 2.2: Integrate Paper Background**
- Add to BinaryData: `Resources/paper_background.jpg`
- Apply as full-size background image

**Task 2.3: Integrate Botanical Overlay (Slug)**
- Add to BinaryData: `Resources/slug.png`
- Position: right side, 0.35 opacity, pointer-events: none

**Task 2.4: Redesign Knobs (Seed Cross-Section)**
- 10-segment conic-gradient pattern
- Warm cream tones with brown dividers
- Inset shadow for depth

**Task 2.5: Restyle Typography**
- Font-family: Garamond, Times New Roman, serif
- Wide letter-spacing on labels
- UPPERCASE transforms
- Text emboss shadow

**Task 2.6: Restyle Buttons/Toggles**
- Green botanical accent colors
- Fleuron decorations (optional)
- Soft rounded corners

**Task 2.7: Update Layout for Naturalist Spacing**
- Generous padding (20-30px)
- Breathable gaps between controls
- Clean visual hierarchy

### Wave 3: Build Integration (CMakeLists.txt)

**Task 3.1: Add Image Resources to BinaryData**
```cmake
juce_add_binary_data(O-Detune_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Resources/paper_background.jpg
        Resources/slug.png
)
```

**Task 3.2: Update Resource Provider (PluginEditor.cpp)**
- Add URL mappings for new image assets
- Serve with correct MIME types (image/jpeg, image/png)

### Wave 4: Verification

**Task 4.1: Parameter Connection Test**
- Verify each parameter changes audio output
- Check automation recording/playback

**Task 4.2: UI Interaction Test**
- Verify all knobs/buttons respond
- Check visual feedback matches parameter state

**Task 4.3: Random LFO Verification**
- Confirm non-repeating pattern over 60+ seconds
- Check smooth transitions between random values

**Task 4.4: Era Character Verification**
- A/B test each era - confirm audible differences
- Document characteristics for user

**Task 4.5: pluginval Validation**
- Run full validation suite
- Address any issues

---

## Algorithm Details

### True Random LFO (Sample-and-Hold with Smoothing)

The random shape must NOT repeat. Implementation:

1. **Sample-and-hold core:** Generate new random target at rate intervals
2. **Smoothing:** Interpolate toward target to avoid clicks
3. **Non-deterministic:** Use system random, not seeded PRNG

```cpp
class RandomLFO {
    float current = 0.0f;
    float target = 0.0f;
    int holdCounter = 0;
    int holdSamples = 0;
    juce::Random rng;

public:
    float processSample(float rate, double sampleRate) {
        if (holdCounter >= holdSamples) {
            target = rng.nextFloat() * 2.0f - 1.0f;  // -1 to +1
            holdSamples = static_cast<int>(sampleRate / rate);
            holdCounter = 0;
        }
        // Smoothing coefficient for ~10ms slew
        const float coeff = 1.0f - std::exp(-1.0f / (0.01f * sampleRate));
        current += (target - current) * coeff;
        holdCounter++;
        return current;
    }
};
```

### Era Character Implementation

| Era | Rate Mult | Depth Mult | Filter | Noise |
|-----|-----------|------------|--------|-------|
| 60s | 0.7x | 0.8x | LP @ 2kHz | None |
| 70s | 1.0x | 1.0x | Bypass | None |
| 80s | 1.3x | 1.1x | HS +3dB @ 4kHz | +5% |

---

## Risk Assessment

| Risk | Mitigation |
|------|------------|
| DSP complexity | Implement incrementally, test each parameter |
| UI regression | Keep original CSS as reference |
| Random LFO clicks | Use smoothing coefficient |
| Performance impact | Profile after changes, optimize if needed |

---

## Acceptance Criteria

1. All 21 parameters affect audio processing
2. UI displays Ouaricon Naturalist aesthetic with paper + slug
3. Random shape is truly non-repeating over 60+ seconds
4. Era selection produces audibly different character
5. pluginval passes all tests
6. Parameter state saves/recalls correctly
7. CPU usage < 5% on Apple Silicon

---

## Estimated Scope

- **DSP changes:** ~400 lines (PluginProcessor.cpp)
- **UI changes:** ~500 lines (index.html complete rewrite)
- **Build changes:** ~10 lines (CMakeLists.txt)
- **Editor changes:** ~30 lines (resource mapping)

---

**Plan Status:** Ready for approval
