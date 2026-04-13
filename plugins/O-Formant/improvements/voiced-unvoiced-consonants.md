# Voiced/Unvoiced Consonant Distinction

## Summary
Add a `consonantVoicing` parameter (0=voiceless, 1=voiced) to the consonant engine so the synth can properly distinguish voiced consonants (B, D, G, V, Z, DH, ZH) from voiceless (P, T, K, F, S, TH, SH). Currently all plosives suppress the glottal source and all fricatives don't — voicing should control this independently from manner.

## Problem
In `ConsonantEngine.h:122`, onset suppression is `(1 - manner)`:
- Plosives (manner=0): full glottal suppression — correct for P/T/K, **wrong for B/D/G** (they need voice bar)
- Fricatives (manner=1): no glottal suppression — correct for V/Z/DH, **wrong for F/S/SH/TH** (they should suppress glottal)

The suppression only lasts 25ms (`onsetTotalSamples`), which is also too short for voiceless fricatives that need glottal suppression for their full 60-120ms duration.

## Implementation

### 1. New APVTS Parameter — `PluginProcessor.cpp`

Add after the `sibilance` parameter (~line 117), before `autoConsonant`:

```cpp
layout.add (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID { "consonantVoicing", 1 },
    "Voicing",
    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
    0.5f));
```

Default 0.5 preserves existing behavior for current patches.

### 2. ConsonantEngine Changes — `dsp/ConsonantEngine.h`

**Add member variables** (near other cached members ~line 254):
```cpp
float cachedVoicing = 0.5f;
```

**Modify `updateCoefficients`** signature to accept voicing:
```cpp
void updateCoefficients (float place, float manner, float voicing, double sr) noexcept
```
Add inside the method body:
```cpp
cachedVoicing = voicing;
```

**Modify onset suppression** in `getNextSample()` (~line 119-124). Change:
```cpp
currentOnsetSuppression = std::exp (-6.0f * onsetProgress)
                          * burstAmplitude * (1.0f - cachedManner);
```
To:
```cpp
// Voiceless: full suppression. Voiced plosive: 70% suppression (voice bar at 30%).
// Voiced fricative: no suppression.
float voicelessFactor = 1.0f - cachedVoicing * (0.3f + 0.7f * cachedManner);
currentOnsetSuppression = std::exp (-6.0f * onsetProgress)
                          * burstAmplitude * voicelessFactor;
```

**Add continuous voiceless suppression** — new method and state. Voiceless fricatives need glottal suppression for the FULL consonant envelope duration, not just the 25ms onset window. Add a public method:
```cpp
// Returns 0-1 suppression factor for voiceless fricatives during full consonant envelope.
// 0 = no suppression, 1 = full suppression.
float getContinuousSuppression() const noexcept
{
    if (consonantPhase == EnvPhase::Off)
        return 0.0f;
    // Scale with how voiceless and how fricative the consonant is
    float fricativeFactor = cachedManner;          // 0 for plosives, 1 for fricatives
    float voicelessFactor = 1.0f - cachedVoicing;  // 1 for voiceless, 0 for voiced
    return fricativeFactor * voicelessFactor;
}
```

Note: `EnvPhase` is private, so this method must be inside the class body (it already is since it's a header-only class).

### 3. FormantVoice Changes — `FormantVoice.h` and `FormantVoice.cpp`

**Add parameter pointer** in `FormantVoice.h` (~line 117, after pSibilance):
```cpp
std::atomic<float>* pConsonantVoicing = nullptr;
```

**Cache it** in `FormantVoice.cpp` `setAPVTS()` (~line 43, after sibilance):
```cpp
pConsonantVoicing = apvts->getRawParameterValue ("consonantVoicing");
```

**Read and pass voicing** in `renderNextBlock()`. Where consonant params are read (~line 317-319), add:
```cpp
float consonantVoicing = pConsonantVoicing != nullptr ? pConsonantVoicing->load() : 0.5f;
```

**Update the consonantEngine call** (~line 323). Change:
```cpp
consonantEngine.updateCoefficients (consonantTone, sibilance, getSampleRate());
```
To:
```cpp
consonantEngine.updateCoefficients (consonantTone, sibilance, consonantVoicing, getSampleRate());
```

**Apply continuous suppression** in the per-sample loop (~line 491-495). Change:
```cpp
float consonantNoise = consonantEngine.getNextSample (consonantLevel);
float onsetSuppression = consonantEngine.getOnsetSuppression();
// During plosive onset, suppress glottal source so noise dominates
float voiceSource = source * (1.0f - 0.7f * onsetSuppression);
```
To:
```cpp
float consonantNoise = consonantEngine.getNextSample (consonantLevel);
float onsetSuppression = consonantEngine.getOnsetSuppression();
float continuousSuppression = consonantEngine.getContinuousSuppression();
// Suppress glottal source: onset burst (plosive) + continuous (voiceless fricative)
float totalSuppression = juce::jmin (1.0f, onsetSuppression + continuousSuppression);
float voiceSource = source * (1.0f - 0.7f * totalSuppression);
```

### 4. WebView UI — `ui/public/index.html` and `ui/public/js/main.js`

**Add relay in `PluginEditor.cpp`** and **`PluginEditor.h`**:
- Declare: `std::unique_ptr<juce::WebSliderRelay> consonantVoicingRelay;`
- Create: `consonantVoicingRelay = std::make_unique<juce::WebSliderRelay> ("consonantVoicingSlider");`
- Add `.withOptionsFrom (*consonantVoicingRelay)` in the WebView options chain
- Add attachment: `consonantVoicingAttachment` (ParameterAttachment for "consonantVoicing")

**Add knob in HTML** — in the consonant section, add a "Voicing" knob alongside Place and Manner.

**Add relay state in JS** — `consonantVoicingState = getSliderState("consonantVoicingSlider")` and wire up a knob element.

### 5. Factory Presets — `PluginProcessor.cpp`

Add `{"consonantVoicing", 0.5f}` to every factory preset definition. Value 0.5 keeps existing sound.

### 6. Consonant XY Pad Label Update (optional)

Consider relabeling the consonant pad:
- X axis: "Place" (labial → velar) — unchanged
- Y axis: currently "Manner" — unchanged  
- New knob below pad: "Voicing" (voiceless ↔ voiced)

## Behavior Summary

| Consonant Type | Place | Manner | Voicing | Glottal Suppression |
|---------------|-------|--------|---------|---------------------|
| P, T, K (voiceless plosive) | varies | 0.0 | 0.0 | Full onset suppression |
| B, D, G (voiced plosive) | varies | 0.0 | 1.0 | Partial (voice bar at ~30%) |
| F, S, SH, TH (voiceless fricative) | varies | 1.0 | 0.0 | Full continuous suppression |
| V, Z, ZH, DH (voiced fricative) | varies | 1.0 | 1.0 | None (noise + voicing) |
| Default (existing patches) | varies | varies | 0.5 | Blended (backward-compatible) |

## Version
MINOR bump — new parameter, no breaking changes. Default 0.5 preserves existing patch sound.

## Testing
1. Set voicing=0, manner=0, place=0.33: should sound like a clean /t/ (burst, no voicing during closure)
2. Set voicing=1, manner=0, place=0.33: should sound like /d/ (burst + low voice bar during closure)
3. Set voicing=0, manner=1, place=0.33: should sound like /s/ (sustained noise, no voicing)
4. Set voicing=1, manner=1, place=0.33: should sound like /z/ (sustained noise + voicing)
5. Load any existing preset: should sound identical to before (voicing defaults to 0.5)
