# O-Formant: Nasals & Liquids Implementation Plan

> Paste this document into a fresh context window. Execute each pass as a separate `/improve` call.
> Current version: v1.14.1 | Target: v1.16.0

---

## Background & Architecture Context

O-Formant is a source-filter vocal synthesizer with:
- **LF glottal source** (wavetable, Rd morphing) -> **aspiration noise** mixing -> **5-formant cascade/parallel/hybrid bank** -> **consonant engine** (noise-based) -> **soft-clip** -> **output**
- **2D vowel morph space** with 5 cardinal vowels (A E I O U) using Shepard IDW interpolation in `VowelMorpher.h`, data in `VowelData.h`
- **CascadeFormantBank** uses all-pole resonators (Klatt 1980) — no spectral zeros
- **ConsonantEngine** handles stops/fricatives via shaped noise with place/manner articulation
- **32 APVTS parameters** currently
- **16-voice polyphony**, ~100 FLOPS/sample/voice

### The Gap

Two sound classes are missing:
1. **Nasals** (m, n, ŋ) — voiced sounds requiring anti-formants (spectral zeros) that the all-pole cascade cannot produce
2. **Liquids** (r, l) — voiced sounds with distinctive formant patterns, especially /r/'s very low F3 (~1600 Hz)

Both are **sonorants**: they use the glottal source (not noise), have continuous voicing, and are sustained sounds. They don't fit the noise-based consonant engine.

---

## Pass 1: Add Liquid Formant Targets (MINOR bump -> v1.15.0)

### Scope
Add /r/ and /l/ (dark variant) as morph points in the existing 2D vowel space. No new DSP filters needed — liquids are modeled purely by formant target interpolation.

### `/improve` command
```
/improve O-Formant add liquid consonant morph points (/r/ and /l/) to the 2D vowel space. This requires changes to VowelData.h, VowelMorpher.h, and the UI (main.js). Details below.
```

### Acoustic Data

**Liquid formant targets (from Espy-Wilson 1992, Stevens 1998):**

| Sound | F1 (Hz) | F2 (Hz) | F3 (Hz) | F4 (Hz) | F5 (Hz) | BW1 | BW2 | BW3 | BW4 | BW5 |
|-------|---------|---------|---------|---------|---------|-----|-----|-----|-----|-----|
| /r/ | 340 | 1050 | 1600 | 3500 | 4300 | 60 | 90 | 130 | 250 | 280 |
| /l/ dark | 400 | 900 | 2600 | 3400 | 4200 | 80 | 120 | 150 | 250 | 280 |

**Gain targets (dB, relative to F1=0dB):**
- /r/: 0, -8, -14, -24, -30 -> linear: 1.0, 0.3981, 0.1995, 0.0631, 0.0316
- /l/ dark: 0, -6, -16, -22, -28 -> linear: 1.0, 0.5012, 0.1585, 0.0794, 0.0398

**XY morph positions** (placed to avoid crowding existing vowels):
- /r/ at (0.12, 0.72) — near /i/ space but distinct (low F1+F2, very low F3)
- /l/ at (0.55, 0.85) — upper-center, between /i/ and /u/ (moderate F1, low F2)

### Files to Modify

**1. `Source/dsp/VowelData.h`**
- Change `kNumVowels = 5` to `kNumVowels = 7`
- Add two new VowelEntry structs after the U entry:
```cpp
// R /r/ - Retroflex approximant (very low F3 signature)
{
    { 340.0f, 1050.0f, 1600.0f, 3500.0f, 4300.0f },   // freq
    { 60.0f,  90.0f,   130.0f,  250.0f,  280.0f },     // bandwidth
    { 1.0f,   0.3981f, 0.1995f, 0.0631f, 0.0316f },    // gain (0,-8,-14,-24,-30 dB)
    0.12f, 0.72f                                         // x, y
},
// L /l/ - Dark lateral approximant (low F2, velarized)
{
    { 400.0f, 900.0f,  2600.0f, 3400.0f, 4200.0f },   // freq
    { 80.0f,  120.0f,  150.0f,  250.0f,  280.0f },     // bandwidth
    { 1.0f,   0.5012f, 0.1585f, 0.0794f, 0.0398f },    // gain (0,-6,-16,-22,-28 dB)
    0.55f, 0.85f                                         // x, y
}
```

**2. `Source/dsp/VowelMorpher.h`**
- No structural changes needed — it already iterates over `VowelData::kNumVowels`. When kNumVowels changes from 5 to 7, the Shepard interpolation automatically includes the new points.

**3. `Source/ui/public/js/main.js`**
- Update the `vowelLabels` array (around line 24) to add the liquid labels:
```js
{ label: 'r',  x: 0.12, y: 0.72 },
{ label: 'l',  x: 0.55, y: 0.85 },
```
- Update the `VOWELS` array (around line 33) to add matching JS data for the spectrum display (if present)
- The XY pad rendering loop already iterates over vowelLabels, so new points appear automatically

### Testing
- Play notes while sweeping vowelX/vowelY toward the new positions
- At (0.12, 0.72) you should hear an "r-colored" tone with F2-F3 convergence
- At (0.55, 0.85) you should hear a dark, velarized "l" quality
- Verify smooth interpolation between vowels and liquids (no clicks/pops)
- Verify existing vowel positions sound identical (regression check)

---

## Pass 2: Add Nasal Pole-Zero Filters + Parameters (MINOR bump -> v1.16.0)

### Scope
Add Klatt-style nasal pole-zero pair + secondary anti-formant notch to the cascade formant chain. Add `nasalCoupling` and `nasalPlace` parameters. Wire into FormantVoice signal chain with aspiration suppression and bandwidth widening during nasal murmurs.

### `/improve` command
```
/improve O-Formant --full add nasal consonant support (m, n, ŋ) via Klatt-style nasal pole-zero filtering. This requires: (1) a new NasalPoleZero.h DSP component with a nasal pole resonator + 2 anti-formant notch filters, (2) two new APVTS parameters (nasalCoupling 0-1, nasalPlace 0-1), (3) integration into CascadeFormantBank's process chain, (4) FormantVoice wiring with aspiration suppression and bandwidth widening when nasalCoupling > 0, (5) UI controls. Full details below.
```

### Acoustic Data

**Nasal pole (shared across all nasals):**
- Frequency: 270 Hz, Bandwidth: 90 Hz (fixed resonance from nasal cavity)

**Anti-formant frequencies by place (nasalPlace parameter maps continuously):**

| nasalPlace | Sound | Anti-F1 (Hz) | Anti-F1 BW | Anti-F2 (Hz) | Anti-F2 BW |
|------------|-------|-------------|------------|-------------|------------|
| 0.0 | /m/ bilabial | 800 | 100 | 2700 | 150 |
| 0.5 | /n/ alveolar | 1700 | 100 | 3500 | 150 |
| 1.0 | /ŋ/ velar | 3200 | 100 | 4800 | 200 |

Interpolate linearly between these three anchor points as nasalPlace sweeps 0-1.

**Nasal murmur formant targets** (when nasalCoupling approaches 1.0, the 5-formant bank should be retuned toward these):

| Formant | Freq (Hz) | BW (Hz) | Gain relative |
|---------|-----------|---------|---------------|
| N1 | 270 | 200 | 0 dB |
| N2 | 1000 | 150 | -8 dB |
| N3 | 2000 | 200 | -15 dB |
| N4 | 2500 | 250 | -20 dB |
| N5 | 3300 | 300 | -30 dB |

### Architecture: NasalPoleZero DSP Component

**New file: `Source/dsp/NasalPoleZero.h`**

This component contains 3 biquad filter stages processed in series:
1. **Nasal pole** — all-pole resonator at 270 Hz, BW 90 Hz (same `makeResonator` formula as CascadeFormantBank). Adds energy at the nasal murmur frequency.
2. **Anti-formant 1** — band-reject (notch) filter at place-dependent frequency. This is the primary nasal place cue.
3. **Anti-formant 2** — band-reject (notch) filter at secondary zero frequency.

**Key behavior:**
- When `nasalCoupling = 0`: the nasal pole frequency = anti-formant 1 frequency (both at 270 Hz). The pole and zero cancel -> flat response -> transparent. Anti-formant 2 is also set to match its pole -> transparent. Zero CPU waste in non-nasal patches.
- When `nasalCoupling > 0`: the anti-formant frequencies interpolate toward their place-specific targets. The nasal pole stays at 270 Hz. The anti-formants create spectral notches.
- Coupling also controls the **depth** of the notch filters (Q of the notches scales with coupling).

**Notch filter formula** (band-reject biquad):
```
For a notch at frequency F with bandwidth BW at sample rate sr:
  w0 = 2*pi*F/sr
  alpha = sin(w0) * sinh(ln(2)/2 * BW/F * w0/sin(w0))
  
  Simplified (use JUCE's makeNotch or compute directly):
  b0 = 1
  b1 = -2*cos(w0)
  b2 = 1
  a0 = 1 + alpha
  a1 = -2*cos(w0)
  a2 = 1 - alpha
  
  Normalize by a0.
```

Or use `juce::dsp::IIR::ArrayCoefficients<float>::makeNotch(sr, freq, Q)` where Q controls notch width.

**Interface:**
```cpp
class NasalPoleZero
{
public:
    void prepare(double sr);
    void reset();
    
    // Call at block-rate (every 32 samples, same as formant coefficient updates)
    // coupling: 0-1 (velum opening)
    // place: 0-1 (bilabial -> alveolar -> velar)
    void updateCoefficients(float coupling, float place, double sr);
    
    // Call per-sample, inline — processes input through pole + 2 anti-formants in series
    inline float process(float input) noexcept;
    
    // Snap SmoothedValues on note onset (same pattern as CascadeFormantBank)
    void snapToTargets();
    
private:
    FormantBiquad nasalPole;      // All-pole resonator at ~270 Hz
    FormantBiquad antiFormant1;   // Primary notch (place cue)
    FormantBiquad antiFormant2;   // Secondary notch
    
    // Smoothed parameters for click-free transitions
    SmoothedValue<float> smoothedAntiFreq1, smoothedAntiFreq2;
    SmoothedValue<float> smoothedAntiQ1, smoothedAntiQ2;
    SmoothedValue<float> smoothedPoleGain;  // Scale pole amplitude with coupling
    
    double sampleRate = 44100.0;
};
```

### Integration Points

**1. `Source/dsp/CascadeFormantBank.h`**
- Add `#include "NasalPoleZero.h"`
- Add `NasalPoleZero nasalPoleZero;` member
- In `prepare()`: call `nasalPoleZero.prepare(sr)`
- In `reset()`: call `nasalPoleZero.reset()`
- In `snapToTargets()`: call `nasalPoleZero.snapToTargets()`
- Add new method: `void updateNasalCoefficients(float coupling, float place, double sr)` that delegates to `nasalPoleZero.updateCoefficients()`
- In `process()`: after the cascade chain output and before returning, run through `nasalPoleZero.process()`:
```cpp
// After existing cascade + parallel sum:
float result = cascadeOut + parallelOut;
// Nasal pole-zero filtering (transparent when coupling=0)
result = nasalPoleZero.process(result);
return result;
```

**2. `Source/PluginProcessor.cpp` — Add 2 new parameters**
In `createParameterLayout()`, add to the Voice Character section:
```cpp
// --- Nasal (2) ---
layout.add (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID { "nasalCoupling", 1 },
    "Nasal Coupling",
    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
    0.0f));  // Default OFF — transparent

layout.add (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID { "nasalPlace", 1 },
    "Nasal Place",
    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
    0.5f));  // Default: alveolar /n/
```

**3. `Source/FormantVoice.h` — Add parameter pointers**
```cpp
// Nasal
std::atomic<float>* pNasalCoupling = nullptr;
std::atomic<float>* pNasalPlace    = nullptr;
```

**4. `Source/FormantVoice.cpp` — Wire nasal DSP**

In `setAPVTS()`:
```cpp
pNasalCoupling = apvts->getRawParameterValue("nasalCoupling");
pNasalPlace    = apvts->getRawParameterValue("nasalPlace");
```

In `renderNextBlock()`, inside the block-rate coefficient update block (the `if ((sampleCounter % kCoeffUpdateInterval) == 0)` section):
```cpp
// Nasal pole-zero coefficients (block-rate)
float nasalCoupling = pNasalCoupling != nullptr ? pNasalCoupling->load() : 0.0f;
float nasalPlace    = pNasalPlace    != nullptr ? pNasalPlace->load()    : 0.5f;

if (topology != 1)  // Cascade or Hybrid
    cascadeBank.updateNasalCoefficients(nasalCoupling, nasalPlace, getSampleRate());

// Nasal bandwidth widening: when coupling > 0, widen formant bandwidths by up to 2x
// This models the increased damping of the nasal cavity walls
if (nasalCoupling > 0.0f)
{
    float bwScale = 1.0f + nasalCoupling * 1.0f;  // 1x at 0, 2x at full nasal
    for (int fi = 0; fi < 5; ++fi)
        formantBWs[fi] *= bwScale;
}
```

For the aspiration noise suppression, in the breathiness calculation section (before `aspirationNoise.setBreathiness()`):
```cpp
// Suppress aspiration during nasal murmurs (nasals are purely voiced, no aspiration)
float nasalCouplingVal = pNasalCoupling != nullptr ? pNasalCoupling->load() : 0.0f;
effectiveBreath *= (1.0f - nasalCouplingVal * 0.8f);  // 80% suppression at full nasal
```

For amplitude reduction during nasals (nasals are ~6-10dB quieter), apply a gain reduction to the voiced path. In the per-sample loop, after the formant filtering:
```cpp
// Nasal amplitude reduction (~8dB at full coupling)
if (nasalCouplingVal > 0.0f)
{
    float nasalGainDb = -8.0f * nasalCouplingVal;
    sample *= juce::Decibels::decibelsToGain(nasalGainDb);
}
```

**5. `Source/ui/public/js/main.js` — Add nasal controls**
- Add relay states for `nasalCouplingState` and `nasalPlaceState`
- Initialize them in `window.__JUCE__.initialisationComplete`
- Add a nasal section to the UI with:
  - A "Nasal Coupling" slider (0-1) — labeled "Nasality" or "Velum"
  - A "Nasal Place" slider (0-1) — labeled with "m...n...ŋ" markers at 0, 0.5, 1.0
- Place in the Voice Character or Consonant section of the UI

### Signal Flow Summary (with nasal integration)

```
Glottal Source
  |
  v
Aspiration Mix (suppressed when nasalCoupling > 0)
  |
  v
Spectral Tilt
  |
  v
Cascade Formant Bank (BWs widened when nasalCoupling > 0)
  |
  v
Nasal Pole-Zero (transparent when nasalCoupling = 0)  <-- NEW
  |
  v
+ Consonant Noise (parallel)
  |
  v
Source-Filter Coupling -> Soft-clip -> Output
```

### Testing
- With nasalCoupling = 0: verify all existing patches sound identical (regression)
- Sweep nasalCoupling from 0 to 1: should hear gradual transition from vowel to nasal murmur
- At nasalCoupling = 1, nasalPlace = 0: should hear /m/ — very dark, muffled, only low-frequency energy
- At nasalCoupling = 1, nasalPlace = 0.5: should hear /n/ — slightly brighter than /m/, mid-frequency gap
- At nasalCoupling = 1, nasalPlace = 1.0: should hear /ŋ/ — richest mid-spectrum of the three nasals
- Sweep nasalPlace while holding a note with nasalCoupling = 1: smooth transition m -> n -> ŋ
- Test with all 3 topologies (cascade, parallel, hybrid)
- Verify no clicks/pops during parameter changes (SmoothedValue ramps)
- CPU usage should increase by < 20% (only 3 extra biquads per voice)

---

## Execution Order

1. `/clear`
2. `/improve O-Formant add liquid consonant morph points /r/ and /l/ to the 2D vowel morph space. Extend VowelData.h from 5 to 7 entries adding /r/ at position (0.12, 0.72) with formants F1=340 F2=1050 F3=1600 F4=3500 F5=4300 Hz, BW 60/90/130/250/280, gains 0/-8/-14/-24/-30 dB, and dark /l/ at position (0.55, 0.85) with formants F1=400 F2=900 F3=2600 F4=3400 F5=4200 Hz, BW 80/120/150/250/280, gains 0/-6/-16/-22/-28 dB. Update vowelLabels and VOWELS arrays in main.js to show 'r' and 'l' labels on the XY pad. VowelMorpher.h already iterates over kNumVowels so it auto-adapts. This is a self-contained change — no new DSP components or parameters needed.`
3. Build, install, test liquids in DAW
4. `/clear`
5. `/improve O-Formant --full add nasal consonant support via Klatt-style nasal pole-zero filtering. Create new file Source/dsp/NasalPoleZero.h containing a nasal pole (all-pole resonator at 270 Hz, BW 90 Hz using same makeResonator formula as CascadeFormantBank) plus two anti-formant notch filters (band-reject biquads). Anti-formant 1 frequency interpolates by nasalPlace: 0.0=800Hz (/m/), 0.5=1700Hz (/n/), 1.0=3200Hz (/ŋ/). Anti-formant 2: 0.0=2700Hz, 0.5=3500Hz, 1.0=4800Hz. Notch BWs ~100-200Hz. When nasalCoupling=0, pole freq = anti-formant freq (cancel = transparent). Add 2 APVTS parameters: nasalCoupling (0-1, default 0) and nasalPlace (0-1, default 0.5). Integrate NasalPoleZero into CascadeFormantBank — process after cascade chain, before return. In FormantVoice: read nasal params at block-rate, call cascadeBank.updateNasalCoefficients(), widen formant bandwidths by up to 2x when coupling > 0, suppress aspiration by 80% when coupling > 0, reduce amplitude by up to 8dB when coupling > 0. Add UI controls in main.js: nasalCoupling slider labeled "Nasality" and nasalPlace slider labeled "Nasal Place" with m/n/ŋ markers. Snap nasal SmoothedValues on note onset. Test all 3 topologies.`
6. Build, install, test nasals in DAW

---

## Risk Notes

- **Parallel topology (topology=1)**: The nasal pole-zero lives inside CascadeFormantBank which isn't used in parallel mode. For parallel topology, the nasal filtering needs to be applied separately in FormantVoice's parallel path. The implementation should check topology and route accordingly — either through cascadeBank (which includes nasal) or through a standalone nasalPoleZero instance for the parallel path.
- **Existing presets**: All existing presets have nasalCoupling=0 (missing param defaults to 0), so they're unaffected. New presets using nasals should be added after both passes complete.
- **Parameter count**: Goes from 32 to 34 — well within APVTS limits.
- **CPU budget**: 3 additional biquads per voice = ~15 FLOPS. At 16 voices, 48kHz, this is negligible (~0.3% single core).
