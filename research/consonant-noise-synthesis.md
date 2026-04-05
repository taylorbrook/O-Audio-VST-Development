---
title: "Consonant Noise Synthesis for Playable Vocal Synthesizer"
created: 2026-04-04
juce_version: "8.0.4"
summary: "Deep technical reference for synthesizing consonant sounds (fricatives, plosives, nasals, sibilants) as musically playable features. Covers filter parameters, noise shaping, burst generation, anti-formants, and real-time control mapping for O-Formant plugin."
domain: dsp
type: research
keywords:
  - consonant-synthesis
  - fricative
  - plosive
  - nasal
  - sibilance
  - noise-shaping
  - klatt
  - vocal-synthesis
  - formant
stages: [0, 1, 2]
agents: [dsp, research]
companion: vocal-formant-synthesis.md
---

# Consonant Noise Synthesis for Playable Vocal Synthesizer

**Comprehensive Technical Reference for O-Formant Plugin**

**Created:** April 2026
**Version:** 1.0
**Companion:** `vocal-formant-synthesis.md` (vowel/formant fundamentals)

---

## Executive Summary

This document provides implementation-level detail for synthesizing consonant sounds as musically playable features in a JUCE-based vocal synthesizer. It covers four consonant families -- fricatives, plosives, nasals, and sibilants -- with specific filter parameters, noise source specifications, timing data, and pseudocode for real-time C++ implementation.

**Key Design Principles:**
- Consonants are synthesized via the **parallel formant branch** (not cascade) in a Klatt-style architecture
- Fricatives use **filtered noise**; plosives use **transient burst noise**; nasals use **anti-formants (zeros)**
- Voiced consonants mix glottal source with noise; unvoiced use noise alone
- Musical control maps consonant injection to note attack, MIDI velocity, mod wheel, and dedicated parameters

---

## Table of Contents

1. [Architecture: Where Consonants Fit](#1-architecture)
2. [Fricative Synthesis](#2-fricative-synthesis)
3. [Plosive / Stop Consonant Synthesis](#3-plosive-synthesis)
4. [Nasal Consonant Synthesis](#4-nasal-synthesis)
5. [Sibilance Modeling](#5-sibilance-modeling)
6. [Musical Control Mapping](#6-musical-control-mapping)
7. [Real-Time Implementation](#7-real-time-implementation)
8. [Complete Consonant Engine Pseudocode](#8-complete-engine)
9. [References](#9-references)

---

## 1. Architecture: Where Consonants Fit {#1-architecture}

### 1.1 Klatt-Style Dual-Branch Topology

The standard approach (Klatt 1980) routes consonants through the **parallel branch**, separate from the cascade vowel path:

```
                    +--> [Cascade Formants F1-F5] --> vowel output
Glottal Source ---->|
                    +--> [Nasal Pole/Zero]       --> nasal contribution
                    |
                    +--> mixed into parallel input (voiced consonants)

                    +--> [Parallel Formants + Bypass] --> fricative/plosive output
Noise Source ------>|
                    +--> direct bypass path       --> flat-spectrum consonants (/f/, /th/)

Final Output = cascade_out + parallel_out + nasal_out
```

### 1.2 Key KLATT Parameters for Consonants

| Parameter | Range | Purpose |
|-----------|-------|---------|
| AF (frication amplitude) | 0-80 dB | Controls noise level into parallel formants |
| AH (aspiration amplitude) | 0-80 dB | Breathiness noise mixed with voicing source |
| AV (voicing amplitude) | 0-80 dB | Glottal source level (0 for unvoiced consonants) |
| A1-A6 (parallel formant amps) | 0-80 dB | Shape the fricative/burst spectrum |
| AB (bypass amplitude) | 0-80 dB | Flat path for /f/, /v/, /th/, /dh/, /p/, /b/ |
| FZ (nasal zero frequency) | 200-800 Hz | Anti-formant for nasal sounds |
| BZ (nasal zero bandwidth) | 40-500 Hz | Width of nasal anti-resonance |
| FP (nasal pole frequency) | 200-500 Hz | Nasal resonance peak |

### 1.3 Branch Routing by Consonant Type

| Consonant Type | Cascade Branch | Parallel Branch | Nasal Branch | Bypass Path |
|----------------|---------------|-----------------|--------------|-------------|
| Vowels | PRIMARY | -- | -- | -- |
| Sibilant fricatives (/s/, /sh/, /z/, /zh/) | -- | PRIMARY | -- | -- |
| Non-sibilant fricatives (/f/, /v/, /th/, /dh/) | -- | -- | -- | PRIMARY |
| Plosive bursts (/p/, /t/, /k/, /b/, /d/, /g/) | -- | PRIMARY (alveolar, velar) | -- | PRIMARY (bilabial) |
| Nasals (/m/, /n/, /ng/) | modified | -- | PRIMARY | -- |
| Voiced consonants | mixed in | mixed in | -- | -- |

---

## 2. Fricative Synthesis {#2-fricative-synthesis}

### 2.1 Noise Source

All fricatives start with a **white noise** source. The spectral character comes entirely from filtering.

```cpp
// Noise generator for fricative source
float generateFricationNoise()
{
    // White noise: uniform random [-1, 1]
    return random.nextFloat() * 2.0f - 1.0f;
}
```

### 2.2 Fricative Filter Parameters

Each fricative has a distinct spectral shape created by bandpass/bandreject filters applied to white noise. The key differentiator is where the spectral energy concentrates.

#### /s/ (Alveolar Sibilant) -- Unvoiced

**Spectral energy:** 3500-10000 Hz, peaks at ~4500 Hz and ~7500 Hz

```
Filter chain:
1. BandReject: center=3000 Hz, BW=1000 Hz  (cut low-mid energy)
2. BandPass:   center=4200 Hz, BW=300 Hz   (emphasize primary peak)
3. BandReject: center=5700 Hz, BW=400 Hz   (create spectral notch)
4. LowPass:    cutoff=11000 Hz              (roll off extreme highs)
```

Parallel formant settings (Klatt-style):
- A2=0, A3=0, A4=40 dB, A5=56 dB, A6=56 dB, AB=0
- AF=60 dB, AV=0

**Key characteristic:** Very high-frequency, "bright" hissing noise. Energy concentrated above 4 kHz.

#### /sh/ (Palato-alveolar Sibilant) -- Unvoiced

**Spectral energy:** 2000-8000 Hz, peak at ~3000-4000 Hz

```
Filter chain:
1. BandReject: center=1000 Hz, BW=500 Hz   (cut low energy)
2. BandPass:   center=3000 Hz, BW=2000 Hz  (broad mid-high emphasis)
3. BandReject: center=4300 Hz, BW=500 Hz   (sculpt spectral shape)
4. BandPass:   center=6500 Hz, BW=1500 Hz  (secondary peak)
5. LowPass:    cutoff=6000 Hz              (earlier rolloff than /s/)
```

Parallel formant settings:
- A2=0, A3=40 dB, A4=50 dB, A5=46 dB, A6=30 dB, AB=0
- AF=60 dB, AV=0

**Key characteristic:** Lower and broader than /s/. The lower cutoff frequency (~2000 Hz vs ~3500 Hz) is the primary distinction.

#### /f/ (Labiodental) -- Unvoiced

**Spectral energy:** Flat/diffuse spectrum, slight emphasis 3000-4000 Hz

```
Filter chain:
1. BandReject: center=4400 Hz, BW=1100 Hz  (slight sculpting)
2. LowPass:    cutoff=11000 Hz             (gentle rolloff)
```

Parallel formant settings:
- A1-A6=0, AB=55 dB (bypass path dominates -- flat spectrum)
- AF=47 dB, AV=0

**Key characteristic:** Very weak, diffuse noise. Uses the **bypass path** because the front cavity (lips to teeth) is too short to create resonances. Amplitude is ~6 dB quieter than sibilants.

#### /th/ (Dental, /theta/) -- Unvoiced

**Spectral energy:** Flat/diffuse, turbulence begins at ~2500 Hz, slight high emphasis at 7500-8000 Hz

```
Filter chain:
Similar to /f/ -- flat spectrum with bypass path
1. HighPass:   cutoff=2500 Hz             (remove low energy)
2. LowPass:    cutoff=11000 Hz            (gentle rolloff)
```

Parallel formant settings:
- A1-A6=0, AB=50 dB
- AF=42 dB, AV=0

**Key characteristic:** Nearly indistinguishable from /f/ in noise shape alone. Perceptual distinction relies on formant transitions into adjacent vowels, not the noise spectrum itself.

### 2.3 Voiced vs. Unvoiced Fricatives

Voiced fricatives (/z/, /v/, /zh/, /dh/) combine the glottal pulse source with fricative noise. The mixing ratio is critical:

```cpp
struct FricativeVoicing
{
    // Relative levels for voiced fricatives (approximate dB values)
    //                    AV (voice)  AF (noise)  Noise source added?
    // /z/ (voiced /s/)     47 dB      50 dB       No (voice only through filters)
    // /v/ (voiced /f/)     48 dB      42 dB       Yes (noise + voice)
    // /zh/ (voiced /sh/)   50 dB      50 dB       Yes (noise + voice)
    // /dh/ (voiced /th/)   48 dB      40 dB       No (voice dominates)
};

float processVoicedFricative(float glottalSample, float noiseSample,
                             float voiceGain, float noiseGain,
                             bool mixNoiseWithVoice)
{
    float source;
    if (mixNoiseWithVoice)
        source = glottalSample * voiceGain + noiseSample * noiseGain;
    else
        source = glottalSample * voiceGain;  // noise routed separately

    // Apply same filter chain as unvoiced counterpart
    return fricativeFilterChain.process(source);
}
```

**Critical insight:** For voiced fricatives, the glottal source passes through the **same filter chain** as the noise. The voice adds pitch/periodicity while the filter chain imposes the same spectral shape.

### 2.4 Fricative Amplitude Envelope

Fricatives have a gradual onset, unlike plosive bursts:

```cpp
// Fricative amplitude ramp: 0 to target in ~90 ms
float fricativeAttackMs = 90.0f;   // gradual onset
float fricativeReleaseMs = 50.0f;  // moderate release

// Envelope generator
class FricativeEnvelope
{
public:
    void trigger(float targetAmplitude, double sampleRate)
    {
        target = targetAmplitude;
        attackIncrement = target / (fricativeAttackMs * 0.001f * (float)sampleRate);
        releaseDecrement = target / (fricativeReleaseMs * 0.001f * (float)sampleRate);
        state = State::Attack;
        currentLevel = 0.0f;
    }

    float process()
    {
        switch (state)
        {
            case State::Attack:
                currentLevel += attackIncrement;
                if (currentLevel >= target)
                {
                    currentLevel = target;
                    state = State::Sustain;
                }
                break;
            case State::Sustain:
                break;
            case State::Release:
                currentLevel -= releaseDecrement;
                if (currentLevel <= 0.0f)
                {
                    currentLevel = 0.0f;
                    state = State::Off;
                }
                break;
            default: break;
        }
        return currentLevel;
    }

    void release() { state = State::Release; }

private:
    enum class State { Off, Attack, Sustain, Release };
    State state = State::Off;
    float currentLevel = 0.0f;
    float target = 0.0f;
    float attackIncrement = 0.0f;
    float releaseDecrement = 0.0f;
};
```

### 2.5 Complete Fricative Preset Table

| Phoneme | Type | Peak Freq (Hz) | Energy Range (Hz) | Noise Level (dB) | Voice Level (dB) | Uses Bypass |
|---------|------|----------------|--------------------|--------------------|--------------------| ------------|
| /s/ | Unvoiced sibilant | 4500, 7500 | 3500-10000 | -15 | 0 | No |
| /z/ | Voiced sibilant | 4500, 7500 | 3500-10000 | -18 | -18 (voice) | No |
| /sh/ | Unvoiced sibilant | 3000-4000 | 2000-8000 | -15 | 0 | No |
| /zh/ | Voiced sibilant | 3000-4000 | 2000-8000 | -15 | -7 (voice) | No |
| /f/ | Unvoiced non-sibilant | Flat (diffuse) | 3000-11000 | -21 | 0 | Yes |
| /v/ | Voiced non-sibilant | Flat (diffuse) | 3000-11000 | -21 | -10 (voice) | Yes |
| /th/ | Unvoiced non-sibilant | Flat (diffuse) | 2500-11000 | -21 | 0 | Yes |
| /dh/ | Voiced non-sibilant | Flat (diffuse) | 2500-11000 | 0 | -9 (voice) | Yes |
| /h/ | Aspiration | ~7000 (broad) | 1000-11000 | via AH param | 0 | No |

---

## 3. Plosive / Stop Consonant Synthesis {#3-plosive-synthesis}

### 3.1 Plosive Structure

A plosive has three temporal phases:
1. **Closure** (silence or voicing bar): 50-120 ms
2. **Burst** (transient noise): 5-25 ms
3. **Aspiration/VOT** (noise trailing into vowel): 0-100 ms

```
Time -->
[--- Closure ---][Burst][--- Aspiration/VOT ---][--- Vowel ---]
     silence      noise    noise + voice         voiced
     (or voicebar)         transition
```

### 3.2 Voice Onset Time (VOT)

VOT is the interval between burst release and onset of voicing. This is the primary cue distinguishing voiced from voiceless stops.

| Stop Pair | Voiced VOT (ms) | Voiceless VOT (ms) | Notes |
|-----------|-----------------|---------------------|-------|
| /b/ vs /p/ | 0-20 | 30-60 | Bilabial |
| /d/ vs /t/ | 0-25 | 60-100 | Alveolar; /t/ has longest aspiration |
| /g/ vs /k/ | 0-20 | 50-80 | Velar |

**Voiced stops** (/b/, /d/, /g/): Voicing begins during or immediately after the burst. VOT near 0 ms.

**Voiceless stops** (/p/, /t/, /k/): Significant aspiration noise between burst and voicing onset. VOT 30-100 ms.

### 3.3 Burst Spectral Shape by Place of Articulation

The burst spectrum is the primary cue for *where* the stop is articulated:

#### Bilabial (/p/, /b/) -- "Diffuse Falling"

**Spectral energy:** Broad, with emphasis 500-1500 Hz, falling toward higher frequencies.

```cpp
struct BilabialBurst
{
    // Burst noise filtered through bypass path (short front cavity)
    // Spectral tilt: negative (more energy at low frequencies)
    static constexpr float burstDurationMs = 10.0f;
    static constexpr float spectralTilt = -6.0f;    // dB/octave (falling)

    // Filter: lowpass-dominated, broad
    // Use bypass path (AB=55 dB) since front cavity is very short
    // A1=45, A2=35, A3=25, A4=15, A5=0, A6=0
};
```

#### Alveolar (/t/, /d/) -- "Diffuse Rising"

**Spectral energy:** Broad, with emphasis above 4000 Hz, rising toward higher frequencies.

```cpp
struct AlveolarBurst
{
    // Burst noise through parallel formants with high-frequency emphasis
    // Spectral tilt: positive (more energy at high frequencies)
    static constexpr float burstDurationMs = 15.0f;
    static constexpr float spectralTilt = +6.0f;    // dB/octave (rising)

    // A1=0, A2=0, A3=30, A4=48, A5=55, A6=55
    // Peak energy around 4000-5000 Hz
};
```

#### Velar (/k/, /g/) -- "Compact Mid-Frequency"

**Spectral energy:** Concentrated peak at 1500-4000 Hz (context-dependent).

```cpp
struct VelarBurst
{
    // Compact burst with prominent mid-frequency peak
    // Peak frequency depends on following vowel:
    //   Before /i/, /e/: ~3000-4700 Hz (fronted)
    //   Before /a/:      ~1500-2000 Hz (backed)
    //   Before /u/, /o/: ~1000-1500 Hz (backed)
    static constexpr float burstDurationMs = 20.0f;  // longest burst

    // A1=0, A2=45, A3=50, A4=30, A5=0, A6=0 (before central vowel)
    // Adjust A2-A4 balance based on vowel context
};
```

### 3.4 Burst Noise Generation

```cpp
class PlosiveBurst
{
public:
    void trigger(PlaceOfArticulation place, bool voiced, float velocity)
    {
        // Burst duration: 5-25 ms depending on place
        float burstMs;
        switch (place)
        {
            case Bilabial:  burstMs = 10.0f; break;
            case Alveolar:  burstMs = 15.0f; break;
            case Velar:     burstMs = 20.0f; break;
        }

        burstSamples = (int)(burstMs * 0.001f * sampleRate);
        burstCounter = 0;
        burstActive = true;

        // Burst amplitude scales with velocity
        burstAmplitude = velocity; // 0.0-1.0 from MIDI velocity

        // VOT: how long aspiration noise continues after burst
        float votMs = voiced ? juce::Random::getSystemRandom().nextFloat() * 20.0f
                             : 30.0f + juce::Random::getSystemRandom().nextFloat() * 70.0f;
        votSamples = (int)(votMs * 0.001f * sampleRate);
        votCounter = 0;

        currentPlace = place;
        isVoiced = voiced;
    }

    float process()
    {
        if (!burstActive) return 0.0f;

        float noise = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        float output = 0.0f;

        if (burstCounter < burstSamples)
        {
            // Burst phase: full noise through spectral shaping
            float envelope = 1.0f;
            // Sharp attack, exponential decay within burst
            float t = (float)burstCounter / (float)burstSamples;
            envelope = std::exp(-3.0f * t); // fast exponential decay
            output = noise * envelope * burstAmplitude;
            burstCounter++;
        }
        else if (votCounter < votSamples)
        {
            // Aspiration/VOT phase: decaying noise (unvoiced stops only)
            if (!isVoiced)
            {
                float t = (float)votCounter / (float)votSamples;
                float envelope = 1.0f - t; // linear decay
                output = noise * envelope * burstAmplitude * 0.5f;
            }
            votCounter++;
        }
        else
        {
            burstActive = false;
        }

        return output;
    }

private:
    double sampleRate = 44100.0;
    int burstSamples = 0, burstCounter = 0;
    int votSamples = 0, votCounter = 0;
    float burstAmplitude = 0.0f;
    bool burstActive = false;
    bool isVoiced = false;
    PlaceOfArticulation currentPlace;
};
```

### 3.5 Formant Transitions (Locus Theory)

When a plosive transitions into a vowel, formant frequencies sweep from the consonant's **locus** to the vowel's steady-state formant values. The F2 locus is the primary place-of-articulation cue:

| Place | F2 Locus (Hz) | F3 Locus (Hz) | Transition Duration (ms) |
|-------|---------------|---------------|--------------------------|
| Bilabial (/p/, /b/) | ~800 | ~2200 | 40-60 |
| Alveolar (/t/, /d/) | ~1800 | ~3000 | 40-60 |
| Velar (/k/, /g/) | ~2000-3000* | ~2200-3000* | 50-80 |

*Velar loci are highly vowel-dependent (velar pinch: F2 and F3 converge).

```cpp
class FormantTransition
{
public:
    void trigger(PlaceOfArticulation place, const VowelPreset& targetVowel,
                 float transitionMs, double sampleRate)
    {
        // Set locus frequencies based on place
        switch (place)
        {
            case Bilabial:
                locusF1 = 200.0f;   // F1 always starts low
                locusF2 = 800.0f;
                locusF3 = 2200.0f;
                break;
            case Alveolar:
                locusF1 = 200.0f;
                locusF2 = 1800.0f;
                locusF3 = 3000.0f;
                break;
            case Velar:
                // Velar: locus depends on target vowel F2
                locusF1 = 200.0f;
                locusF2 = 0.7f * targetVowel.f[1] + 0.3f * 2300.0f; // biased toward vowel
                locusF3 = locusF2 + 200.0f; // F2-F3 convergence ("velar pinch")
                break;
        }

        targetF1 = targetVowel.f[0];
        targetF2 = targetVowel.f[1];
        targetF3 = targetVowel.f[2];

        transitionSamples = (int)(transitionMs * 0.001f * sampleRate);
        counter = 0;
        active = true;
    }

    // Returns current formant frequencies during transition
    void getCurrentFormants(float& f1, float& f2, float& f3)
    {
        if (!active)
        {
            f1 = targetF1; f2 = targetF2; f3 = targetF3;
            return;
        }

        float t = (float)counter / (float)transitionSamples;
        t = juce::jlimit(0.0f, 1.0f, t);
        // Cosine interpolation for smooth transition
        t = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * t));

        // Log-frequency interpolation
        f1 = std::exp(std::log(locusF1) * (1.0f - t) + std::log(targetF1) * t);
        f2 = std::exp(std::log(locusF2) * (1.0f - t) + std::log(targetF2) * t);
        f3 = std::exp(std::log(locusF3) * (1.0f - t) + std::log(targetF3) * t);

        counter++;
        if (counter >= transitionSamples)
            active = false;
    }

private:
    float locusF1, locusF2, locusF3;
    float targetF1, targetF2, targetF3;
    int transitionSamples = 0, counter = 0;
    bool active = false;
};
```

### 3.6 Complete Plosive Preset Table

| Phoneme | Voiced | Burst Freq Range (Hz) | Burst Duration (ms) | VOT (ms) | Spectral Tilt | Bypass |
|---------|--------|-----------------------|---------------------|-----------|---------------|--------|
| /p/ | No | 500-1500 (diffuse falling) | 10 | 30-60 | -6 dB/oct | Yes |
| /b/ | Yes | 500-1500 (diffuse falling) | 10 | 0-20 | -6 dB/oct | Yes |
| /t/ | No | 4000+ (diffuse rising) | 15 | 60-100 | +6 dB/oct | No |
| /d/ | Yes | 4000+ (diffuse rising) | 15 | 0-25 | +6 dB/oct | No |
| /k/ | No | 1500-4000 (compact) | 20 | 50-80 | ~0 (peaked) | No |
| /g/ | Yes | 1500-4000 (compact) | 20 | 0-20 | ~0 (peaked) | No |

---

## 4. Nasal Consonant Synthesis {#4-nasal-synthesis}

### 4.1 Nasal Acoustics Overview

Nasal consonants occur when the velum (soft palate) lowers, coupling the nasal cavity to the vocal tract. This creates:

- **Nasal poles (formants):** Resonances of the nasal passage, adding energy
- **Nasal zeros (anti-formants):** Destructive interference from the oral side-branch, removing energy

The result is a characteristic "muffled" sound with strong low-frequency energy and attenuated mid-frequencies.

### 4.2 Nasal Resonance Frequencies

All nasals share these nasal tract resonances:

| Resonance | Frequency (Hz) | Bandwidth (Hz) | Notes |
|-----------|---------------|-----------------|-------|
| N1 (nasal murmur) | 250-300 | 80-100 | Very strong; defines the nasal "hum" |
| N2 | 1000-1300 | 90-120 | Weaker; varies by place |
| N3 | ~2000 | 150-200 | Weak; sometimes absent |
| Nasal tract main | ~400 | 100 | Pharynx + nasal tract resonance |
| Nasal tract secondary | ~1200 | 150 | Second nasal tract resonance |
| Nasal tract tertiary | ~2000 | 200 | Third nasal tract resonance |

### 4.3 Anti-Formant (Zero) Placement by Place of Articulation

The oral cavity behind the closure acts as a side-branch resonator. Its length determines where anti-formants appear -- this is how we distinguish /m/, /n/, /ng/:

| Nasal | Place | First Zero (Hz) | Zero BW (Hz) | Second Zero (Hz) | Perceptual Effect |
|-------|-------|-----------------|--------------|-------------------|-------------------|
| /m/ | Bilabial | 750-1250 | 80-120 | ~2500 | Zeros near F2 region; very muffled |
| /n/ | Alveolar | 1450-2200 | 80-150 | 3000-4000 | Zeros attenuate F2 and F4 |
| /ng/ | Velar | 3000+ | 100-200 | -- | Zeros only at high frequencies; clearer |

**Key insight:** /m/ has the longest oral side-branch (entire mouth), so its zeros are at the lowest frequencies. /ng/ has almost no oral side-branch (closure at velum), so zeros are very high and less perceptually important.

### 4.4 Cascade vs. Parallel Topology for Nasals

Nasals require **anti-formants (zeros)**, which can only be modeled in two ways:

**Option A: Cascade with nasal pole-zero pair (recommended)**

```cpp
// The Klatt approach: add a pole-zero pair to the cascade formant chain
class NasalPoleZero
{
public:
    void setNasal(float poleFreq, float poleBW, float zeroFreq, float zeroBW, double sr)
    {
        // Nasal pole (adds resonance)
        float wpole = 2.0f * pi * poleFreq / (float)sr;
        float rpole = std::exp(-pi * poleBW / (float)sr);
        poleA1 = -2.0f * rpole * std::cos(wpole);
        poleA2 = rpole * rpole;

        // Nasal zero (removes resonance)
        float wzero = 2.0f * pi * zeroFreq / (float)sr;
        float rzero = std::exp(-pi * zeroBW / (float)sr);
        zeroB0 = 1.0f;
        zeroB1 = 2.0f * rzero * std::cos(wzero);
        zeroB2 = -(rzero * rzero);
    }

    float process(float input)
    {
        // Zero section (FIR part)
        float zeroOut = zeroB0 * input + zeroB1 * zeroState1 + zeroB2 * zeroState2;
        zeroState2 = zeroState1;
        zeroState1 = input;

        // Pole section (IIR part)
        float poleOut = zeroOut - poleA1 * poleState1 - poleA2 * poleState2;
        poleState2 = poleState1;
        poleState1 = poleOut;

        return poleOut;
    }

private:
    float poleA1 = 0, poleA2 = 0;
    float zeroB0 = 1, zeroB1 = 0, zeroB2 = 0;
    float poleState1 = 0, poleState2 = 0;
    float zeroState1 = 0, zeroState2 = 0;
    static constexpr float pi = juce::MathConstants<float>::pi;
};
```

**Option B: Parallel with nasal amplitude (simpler, less accurate)**

```cpp
// Simpler approach: dedicated nasal formant in parallel
// Use A_nasal parameter to control nasal resonance contribution
float nasalContribution = nasalFormant.process(voicedSource) * nasalAmplitude;
// Subtract anti-formant effect by notch-filtering the main output
float mainOutput = notchFilter.process(cascadeOutput);
output = mainOutput + nasalContribution;
```

**Recommendation for O-Formant:** Use Option A (cascade pole-zero) for realistic nasals, but also expose a simpler "nasality" knob that just crossfades the zero depth for musical control.

### 4.5 Nasal Consonant Presets

```cpp
struct NasalPreset
{
    float poleFreq;      // Nasal pole (Hz) -- always ~300 Hz
    float poleBW;        // Nasal pole bandwidth
    float zeroFreq;      // Anti-formant frequency (Hz)
    float zeroBW;        // Anti-formant bandwidth
    float n1Freq;        // Nasal murmur frequency (Hz)
    float n1BW;          // Nasal murmur bandwidth
};

const NasalPreset nasalM = { 300, 90, 1000, 100, 270, 80 };  // /m/ bilabial
const NasalPreset nasalN = { 300, 90, 1700, 120, 280, 80 };  // /n/ alveolar
const NasalPreset nasalNG = { 300, 90, 3200, 150, 280, 80 }; // /ng/ velar
```

### 4.6 Nasal Murmur Signal Flow

During a nasal consonant, the sound source is the **voiced glottal pulse** (nasals are always voiced), filtered through:

```
Glottal Source --> [Nasal Pole-Zero] --> [Cascade Formants (modified)] --> output
                                              |
                                   F1=250-300 Hz (very low)
                                   F2=attenuated by anti-formant
                                   F3=~2500 Hz (visible)
```

```cpp
float processNasalConsonant(float glottalSample, const NasalPreset& preset)
{
    // Nasal murmur: strong F1 around 250-300 Hz, weak higher formants
    float nasalMurmur = nasalPoleZero.process(glottalSample);

    // Apply very low F1 and attenuated F2
    formantFilters[0].setFormant(preset.n1Freq, preset.n1BW, 0.0f, sampleRate);
    formantFilters[1].setFormant(1000.0f, 200.0f, -15.0f, sampleRate);  // weak F2
    formantFilters[2].setFormant(2500.0f, 150.0f, -10.0f, sampleRate);  // visible F3

    float filtered = nasalMurmur;
    for (int i = 0; i < 3; ++i)
        filtered = formantFilters[i].process(filtered);

    return filtered;
}
```

---

## 5. Sibilance Modeling {#5-sibilance-modeling}

### 5.1 Sibilance Frequency Bands

Sibilants (/s/, /z/, /sh/, /zh/) are the highest-energy consonants and the most musically useful for adding "presence" and "air":

| Sibilant | Primary Band (Hz) | Peak Frequency (Hz) | Secondary Band (Hz) | Character |
|----------|-------------------|---------------------|---------------------|-----------|
| /s/ | 4000-10000 | 4500, 7500 | 2000-4000 (weak) | Bright, piercing hiss |
| /z/ | 4000-10000 | 4500, 7500 | + voice F0 | Voiced bright hiss |
| /sh/ | 2000-7000 | 3000-4000 | 6000-8000 (weak) | Darker, broader "shush" |
| /zh/ | 2000-7000 | 3000-4000 | + voice F0 | Voiced darker hiss |

### 5.2 Sibilance vs. Formant Interaction

Sibilants are generated in the **parallel branch** and summed with the **cascade branch** output (formant/vowel). The interaction matters:

```
                                    Parallel Branch
                                    (sibilant noise)
                                          |
Cascade Branch ----+----[Sum]-----> Output
(vowel formants)   |
                   |
                   +-- Sibilant DOES NOT pass through formant filters
                       It has its OWN spectral shape
```

**Important:** Sibilant noise should NOT be routed through the vowel formant filters. Doing so would impose vowel coloration on the consonant, which sounds unnatural. The parallel branch exists precisely so that consonant noise retains its own spectral identity.

However, for a **musical** vocal synth (not speech), allowing partial routing of sibilant noise through the formant filters can create interesting "vowel-colored hiss" effects. This could be a user-controllable parameter:

```cpp
float sibilanceFormantBleed = 0.0f; // 0.0 = pure sibilant, 1.0 = fully formant-colored

float parallelOut = sibilantFilters.process(noise);
float cascadeBleed = formantBank.process(noise) * sibilanceFormantBleed;
float sibilantOutput = parallelOut * (1.0f - sibilanceFormantBleed) + cascadeBleed;
```

### 5.3 Gender-Dependent Sibilance

Sibilance frequency ranges shift with speaker size:

| Speaker | /s/ Peak (Hz) | /sh/ Peak (Hz) | Notes |
|---------|---------------|-----------------|-------|
| Male | 4000-6000 | 2500-4000 | Lower sibilance |
| Female | 6000-8000 | 3500-6000 | Higher sibilance |
| Child | 7000-9000 | 4000-7000 | Highest sibilance |

This can be tied to a "character" or "gender" parameter that scales sibilant filter center frequencies.

### 5.4 Sibilant Filter Implementation

```cpp
class SibilantFilter
{
public:
    enum Type { S_Sound, Sh_Sound };

    void prepare(double sampleRate, Type type)
    {
        sr = sampleRate;
        switch (type)
        {
            case S_Sound:
                // /s/: Two peaks at 4500 and 7500 Hz
                peak1.setCoefficients(
                    juce::IIRCoefficients::makePeakFilter(sr, 4500.0, 2.0, 6.0));
                peak2.setCoefficients(
                    juce::IIRCoefficients::makePeakFilter(sr, 7500.0, 2.5, 4.0));
                highpass.setCoefficients(
                    juce::IIRCoefficients::makeHighPass(sr, 3500.0));
                lowpass.setCoefficients(
                    juce::IIRCoefficients::makeLowPass(sr, 11000.0));
                break;

            case Sh_Sound:
                // /sh/: Broad peak at 3000-4000 Hz
                peak1.setCoefficients(
                    juce::IIRCoefficients::makePeakFilter(sr, 3500.0, 1.0, 8.0));
                peak2.setCoefficients(
                    juce::IIRCoefficients::makePeakFilter(sr, 6000.0, 2.0, 2.0));
                highpass.setCoefficients(
                    juce::IIRCoefficients::makeHighPass(sr, 2000.0));
                lowpass.setCoefficients(
                    juce::IIRCoefficients::makeLowPass(sr, 8000.0));
                break;
        }
    }

    float process(float noise)
    {
        float out = highpass.processSingleSampleRaw(noise);
        out = peak1.processSingleSampleRaw(out);
        out = peak2.processSingleSampleRaw(out);
        out = lowpass.processSingleSampleRaw(out);
        return out;
    }

private:
    double sr;
    juce::IIRFilter peak1, peak2, highpass, lowpass;
};
```

---

## 6. Musical Control Mapping {#6-musical-control-mapping}

### 6.1 Design Philosophy

Speech synthesis controls consonants with dozens of parameters changing every 5 ms. A musical instrument needs to reduce this to a handful of expressive controls. The recommended approach:

**Tier 1 -- Essential controls (always visible):**
- **Consonant Amount** (0-100%): Master mix of consonant injection
- **Consonant Type** selector: Fricative / Plosive / Nasal / Sibilant
- **Articulation** (0-100%): Sharpness/definition of consonants

**Tier 2 -- Expressive controls (MIDI-mappable):**
- **Sibilance** (0-100%): Amount of /s/-/sh/ type hiss
- **Breathiness** (0-100%): Aspiration noise (overlaps with consonant)
- **Attack Consonant** selector: Which consonant triggers on note-on

**Tier 3 -- Advanced (hidden in detail panel):**
- Individual fricative/plosive mix
- Formant transition speed
- VOT control
- Nasal amount

### 6.2 MIDI Mapping Strategy

```cpp
// Recommended MIDI CC assignments
enum ConsonantMidiCC
{
    CC_ConsonantAmount  = 1,   // Mod wheel -- most natural for consonant control
    CC_Sibilance        = 74,  // Brightness/Cutoff -- semantically appropriate
    CC_Articulation     = 71,  // Timbre/Resonance
    CC_Breathiness      = 2,   // Breath controller (actual breath controller CC)
    CC_NasalAmount      = 75   // General purpose
};

// Velocity mapping
struct VelocityMapping
{
    // MIDI velocity controls consonant behavior:
    // Low velocity (0-40):   Soft attack, minimal consonant, no plosive burst
    // Mid velocity (40-100): Normal attack, moderate consonant
    // High velocity (100-127): Hard attack, strong plosive burst, sibilant emphasis

    float getConsonantIntensity(int velocity)
    {
        return std::pow((float)velocity / 127.0f, 1.5f); // slight exponential curve
    }

    float getPlosiveBurstLevel(int velocity)
    {
        // Only trigger burst above velocity threshold
        if (velocity < 40) return 0.0f;
        return ((float)velocity - 40.0f) / 87.0f;
    }

    float getSibilanceLevel(int velocity)
    {
        return (float)velocity / 127.0f * sibilanceKnob;
    }
};
```

### 6.3 Envelope-Triggered Consonants

The most natural approach: consonants are **attack-phase phenomena**. The note envelope should trigger consonant injection automatically:

```cpp
class ConsonantEnvelopeIntegration
{
public:
    void noteOn(int midiNote, int velocity, ConsonantType type)
    {
        currentConsonant = type;
        float intensity = velocityMapping.getConsonantIntensity(velocity);

        switch (type)
        {
            case ConsonantType::Plosive:
            {
                // Plosive: immediate burst, then formant transition into vowel
                plosiveBurst.trigger(currentPlace, isVoiced, intensity);
                formantTransition.trigger(currentPlace, currentVowel, 50.0f, sampleRate);
                // Mute vowel during burst, ramp in during VOT
                vowelMuteMs = plosiveBurst.getVOTDuration();
                break;
            }
            case ConsonantType::Fricative:
            {
                // Fricative: overlaps with vowel onset for 50-100 ms
                fricativeEnvelope.trigger(intensity, sampleRate);
                fricativeDurationMs = 80.0f + consonantAmountKnob * 120.0f; // 80-200 ms
                break;
            }
            case ConsonantType::Nasal:
            {
                // Nasal: 50-80 ms of nasal murmur before vowel opens
                nasalDurationMs = 60.0f + consonantAmountKnob * 60.0f;
                nasalActive = true;
                break;
            }
            case ConsonantType::Sibilant:
            {
                // Sibilant: overlap with attack, duration from knob
                sibilantEnvelope.trigger(intensity * sibilanceKnob, sampleRate);
                break;
            }
        }
    }

    // Called every sample during voice processing
    float processConsonant(float glottalSource, float noiseSource)
    {
        float consonantOut = 0.0f;

        if (plosiveBurst.isActive())
            consonantOut += burstFilter.process(plosiveBurst.process());

        if (fricativeEnvelope.isActive())
            consonantOut += fricativeFilter.process(noiseSource) * fricativeEnvelope.process();

        if (nasalActive)
            consonantOut += processNasal(glottalSource);

        if (sibilantEnvelope.isActive())
            consonantOut += sibilantFilter.process(noiseSource) * sibilantEnvelope.process();

        return consonantOut;
    }
};
```

### 6.4 Making Consonants Musical (Not Just Speech-Like)

Key design decisions for musical rather than speech-like consonants:

1. **Sustained sibilance:** In speech, /s/ lasts 80-150 ms. Musically, allow infinite sustain of sibilant noise as a texture layer. Think of it as a built-in noise oscillator shaped like speech.

2. **Rhythmic plosives:** Allow plosive bursts to retrigger on every note without full closure silence. This creates percussion-like articulation.

3. **Nasal as resonance color:** Rather than full nasal consonants, use the nasal pole-zero as a timbral modifier -- a "nasality" knob that adds nasal character to sustained vowels.

4. **Vowel-consonant morph:** A single knob that smoothly transitions from pure vowel to pure consonant noise, with the filter chain morphing between formant peaks and fricative bands.

5. **Consonant as envelope follower:** Map consonant amount to the amplitude envelope so consonants naturally appear in attack transients and fade during sustain, mimicking how singers naturally articulate.

```cpp
// Example: consonant-vowel morph parameter
float consonantVowelMix = 0.0f; // 0.0 = pure vowel, 1.0 = pure consonant

float morphedOutput(float vowelSignal, float consonantSignal, float ampEnvelope)
{
    // Auto-consonant: consonant stronger during attack
    float envFollower = 1.0f - ampEnvelope; // high during attack (envelope rising)
    float autoConsonant = consonantVowelMix * (0.3f + 0.7f * envFollower);

    return vowelSignal * (1.0f - autoConsonant) + consonantSignal * autoConsonant;
}
```

---

## 7. Real-Time Implementation {#7-real-time-implementation}

### 7.1 CPU Cost Analysis

| Component | Filters per Voice | Operations/Sample | Relative Cost |
|-----------|-------------------|-------------------|---------------|
| Vowel (cascade 5 formants) | 5 x biquad | ~50 mul + 50 add | 1.0x (baseline) |
| Fricative (parallel) | 4-6 x biquad | ~60 mul + 60 add | 1.2x |
| Plosive burst | 2-3 x biquad + envelope | ~30 mul + 30 add | 0.6x (but transient) |
| Nasal pole-zero | 1 x biquad + 1 x FIR2 | ~15 mul + 15 add | 0.3x |
| Sibilant filter | 4 x biquad | ~40 mul + 40 add | 0.8x |
| **Total consonant overhead** | | | **~2-3x vowel cost** |

**At 8 voices, 44.1 kHz:** ~(50 + 60 + 30 + 15 + 40) * 8 * 44100 = ~69M ops/sec. Trivial for modern CPUs.

### 7.2 State Management for Transients

Plosive bursts are the trickiest -- they're one-shot transients that must be sample-accurate:

```cpp
class ConsonantStateManager
{
public:
    // Per-voice consonant state
    struct VoiceConsonantState
    {
        PlosiveBurst burst;
        FricativeEnvelope fricEnv;
        SibilantFilter sibilant;
        NasalPoleZero nasalPZ;
        FormantTransition transition;

        bool plosiveActive = false;
        bool fricativeActive = false;
        bool nasalActive = false;
        bool sibilantActive = false;

        // Reset all consonant state when voice is stolen
        void reset()
        {
            burst = PlosiveBurst();
            fricEnv = FricativeEnvelope();
            nasalPZ = NasalPoleZero();
            transition = FormantTransition();
            plosiveActive = false;
            fricativeActive = false;
            nasalActive = false;
            sibilantActive = false;
        }
    };

    VoiceConsonantState voices[MAX_VOICES];

    void stealVoice(int voiceIndex)
    {
        voices[voiceIndex].reset(); // clean slate -- no residual consonant artifacts
    }
};
```

### 7.3 Polyphonic Considerations

Each voice needs its own complete consonant state. Shared noise sources are acceptable:

```cpp
class PolyphonicConsonantEngine
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Generate ONE noise sample, shared across voices
            float sharedNoise = noiseGen.nextFloat() * 2.0f - 1.0f;

            float mixL = 0.0f, mixR = 0.0f;

            for (int v = 0; v < activeVoices; ++v)
            {
                auto& voice = voices[v];

                // Each voice gets its own filtered copy of shared noise
                float voiceNoise = sharedNoise;

                // Generate glottal source for this voice (pitch-dependent)
                float glottal = voice.glottalSource.process(voice.frequency);

                // Vowel path (cascade)
                float vowelOut = voice.cascadeFormants.process(glottal);

                // Consonant path
                float consonantOut = voice.consonantState.processConsonant(
                    glottal, voiceNoise);

                // Apply formant transition if active
                if (voice.consonantState.transition.isActive())
                {
                    float f1, f2, f3;
                    voice.consonantState.transition.getCurrentFormants(f1, f2, f3);
                    voice.cascadeFormants.updateFormants(f1, f2, f3);
                }

                // Mix vowel + consonant
                float voiceOut = vowelOut + consonantOut;
                voiceOut *= voice.ampEnvelope.process();

                mixL += voiceOut;
                mixR += voiceOut;
            }

            buffer.getWritePointer(0)[sample] += mixL;
            buffer.getWritePointer(1)[sample] += mixR;
        }
    }

private:
    juce::Random noiseGen;
    int activeVoices = 0;
    static constexpr int MAX_VOICES = 16;
    Voice voices[MAX_VOICES];
};
```

### 7.4 Avoiding Clicks and Artifacts

Consonant transitions are the most click-prone part of a vocal synth:

```cpp
// 1. Always crossfade when switching consonant types
void switchConsonant(ConsonantType newType, float crossfadeMs = 5.0f)
{
    // Don't hard-switch -- crossfade over 5 ms
    crossfadeEnvelope.start(crossfadeMs, sampleRate);
    pendingConsonant = newType;
    // Old consonant fades out, new one fades in
}

// 2. Plosive bursts: apply micro-envelope to first 0.5 ms
float applyBurstOnsetSmoothing(float burstSample, int sampleIndex)
{
    int rampSamples = (int)(0.0005f * sampleRate); // 0.5 ms
    if (sampleIndex < rampSamples)
        return burstSample * (float)sampleIndex / (float)rampSamples;
    return burstSample;
}

// 3. Nasal pole-zero: smooth parameter changes over 2-3 ms
void updateNasalParameters(float newZeroFreq, float smoothMs = 3.0f)
{
    // Never update pole-zero coefficients instantaneously
    nasalZeroFreqSmoothed.setTargetValue(newZeroFreq);
    nasalZeroFreqSmoothed.reset(sampleRate, smoothMs * 0.001);
}
```

---

## 8. Complete Consonant Engine Pseudocode {#8-complete-engine}

### 8.1 ConsonantEngine Class Outline

```cpp
class ConsonantEngine
{
public:
    // === TYPES ===
    enum class ConsonantCategory { None, Fricative, Plosive, Nasal, Sibilant };

    enum class FricativeType { S, Sh, F, V, Z, Th, Dh, Zh };
    enum class PlosiveType   { P, B, T, D, K, G };
    enum class NasalType     { M, N, NG };
    enum class SibilantType  { S, Sh };

    // === PARAMETERS (from APVTS) ===
    struct Parameters
    {
        float consonantAmount;      // 0-1: master consonant mix
        float sibilance;            // 0-1: sibilant noise level
        float nasality;             // 0-1: nasal character
        float articulation;         // 0-1: consonant sharpness/speed
        float breathiness;          // 0-1: aspiration noise
        ConsonantCategory attackConsonant;
        FricativeType fricativeType;
        PlosiveType plosiveType;
        NasalType nasalType;
    };

    // === LIFECYCLE ===
    void prepare(double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;

        // Initialize per-voice consonant state
        for (auto& v : voiceStates)
        {
            v.fricativeFilter.prepare(sampleRate);
            v.sibilantFilter.prepare(sampleRate, SibilantFilter::S_Sound);
            v.burst.prepare(sampleRate);
            v.nasalPZ.prepare(sampleRate);
            v.fricEnv.prepare(sampleRate);
        }

        // Shared noise buffer
        noiseBuffer.setSize(1, maxBlockSize);
    }

    // === NOTE EVENTS ===
    void noteOn(int voiceIndex, int midiNote, int velocity, const Parameters& params)
    {
        auto& v = voiceStates[voiceIndex];
        float intensity = std::pow((float)velocity / 127.0f, 1.5f) * params.consonantAmount;

        switch (params.attackConsonant)
        {
            case ConsonantCategory::Plosive:
                v.burst.trigger(getPlace(params.plosiveType),
                               isVoiced(params.plosiveType), intensity);
                v.transition.trigger(getPlace(params.plosiveType),
                                     getCurrentVowel(), getTransitionMs(params), sr);
                break;

            case ConsonantCategory::Fricative:
                configureFricativeFilter(v.fricativeFilter, params.fricativeType);
                v.fricEnv.trigger(intensity, sr);
                break;

            case ConsonantCategory::Nasal:
                configureNasal(v.nasalPZ, params.nasalType);
                v.nasalActive = true;
                v.nasalDuration = (int)(0.060f * sr * (1.0f + params.articulation));
                v.nasalCounter = 0;
                break;

            case ConsonantCategory::Sibilant:
                v.sibilantFilter.prepare(sr,
                    params.fricativeType == FricativeType::Sh
                        ? SibilantFilter::Sh_Sound : SibilantFilter::S_Sound);
                v.sibilantEnv.trigger(intensity * params.sibilance, sr);
                break;

            default: break;
        }

        // Always-on sibilance layer (if sibilance knob > 0)
        if (params.sibilance > 0.01f)
        {
            v.sibilantLayerLevel = params.sibilance * intensity * 0.3f;
        }
    }

    void noteOff(int voiceIndex)
    {
        auto& v = voiceStates[voiceIndex];
        v.fricEnv.release();
        v.sibilantEnv.release();
        v.nasalActive = false;
    }

    // === PER-SAMPLE PROCESSING ===
    float processSample(int voiceIndex, float glottalSource, float noise,
                        const Parameters& params, float ampEnvelope)
    {
        auto& v = voiceStates[voiceIndex];
        float out = 0.0f;

        // Plosive burst
        if (v.burst.isActive())
        {
            float burstNoise = v.burst.process();
            out += v.burstFilter.process(burstNoise);
        }

        // Fricative
        if (v.fricEnv.isActive())
        {
            float fricSource = noise;
            // Voiced fricatives: mix glottal with noise
            if (isVoicedFricative(params.fricativeType))
                fricSource = noise * 0.5f + glottalSource * 0.5f;
            out += v.fricativeFilter.process(fricSource) * v.fricEnv.process();
        }

        // Nasal
        if (v.nasalActive && v.nasalCounter < v.nasalDuration)
        {
            out += v.nasalPZ.process(glottalSource) * params.nasality;
            v.nasalCounter++;
            if (v.nasalCounter >= v.nasalDuration)
                v.nasalActive = false;
        }

        // Sibilant (attack-triggered + sustained layer)
        float sibilantOut = 0.0f;
        if (v.sibilantEnv.isActive())
            sibilantOut += v.sibilantFilter.process(noise) * v.sibilantEnv.process();

        // Sustained sibilant layer: follows amplitude envelope
        sibilantOut += v.sibilantFilter.process(noise) * v.sibilantLayerLevel
                     * (1.0f - ampEnvelope); // more sibilance during attack/release

        out += sibilantOut;

        return out * params.consonantAmount;
    }

private:
    double sr = 44100.0;

    struct VoiceConsonantState
    {
        PlosiveBurst burst;
        FricativeEnvelope fricEnv;
        FricativeEnvelope sibilantEnv;
        SibilantFilter sibilantFilter;
        FricativeFilterChain fricativeFilter;
        BurstSpectralFilter burstFilter;
        NasalPoleZero nasalPZ;
        FormantTransition transition;

        bool nasalActive = false;
        int nasalDuration = 0;
        int nasalCounter = 0;
        float sibilantLayerLevel = 0.0f;

        void reset()
        {
            burst = PlosiveBurst();
            fricEnv = FricativeEnvelope();
            sibilantEnv = FricativeEnvelope();
            nasalPZ = NasalPoleZero();
            transition = FormantTransition();
            nasalActive = false;
            sibilantLayerLevel = 0.0f;
        }
    };

    VoiceConsonantState voiceStates[16];
    juce::AudioBuffer<float> noiseBuffer;
};
```

### 8.2 APVTS Parameter Layout

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createConsonantParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Tier 1: Essential
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "consonantAmount", "Consonant Amount", 0.0f, 1.0f, 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "consonantType", "Consonant Type",
        juce::StringArray{"None", "Fricative", "Plosive", "Nasal", "Sibilant"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "articulation", "Articulation", 0.0f, 1.0f, 0.5f));

    // Tier 2: Expressive
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sibilance", "Sibilance", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "breathiness", "Breathiness", 0.0f, 1.0f, 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "nasality", "Nasality", 0.0f, 1.0f, 0.0f));

    // Tier 3: Detail
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "fricativeType", "Fricative Type",
        juce::StringArray{"S", "Sh", "F", "V", "Z", "Th"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "plosiveType", "Plosive Type",
        juce::StringArray{"P", "B", "T", "D", "K", "G"}, 2)); // default: /t/

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "nasalType", "Nasal Type",
        juce::StringArray{"M", "N", "NG"}, 1)); // default: /n/

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "formantTransitionSpeed", "Transition Speed", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "voiceOnsetTime", "VOT", 0.0f, 1.0f, 0.5f));

    return { params.begin(), params.end() };
}
```

---

## 9. References {#9-references}

### Primary Sources

1. **Klatt, D.H. (1980).** "Software for a cascade/parallel formant synthesizer." *Journal of the Acoustical Society of America*, 67(3), 971-995. The foundational reference for parametric consonant synthesis with cascade/parallel architecture.

2. **Stevens, K.N. & Blumstein, S.E. (1978).** "Invariant cues for place of articulation in stop consonants." *JASA*, 64(5), 1358-1368. Defines the diffuse-falling (bilabial), diffuse-rising (alveolar), compact (velar) burst spectral templates.

3. **Narayanan, S. & Alwan, A. (2000).** "Noise source models for fricative consonants." *IEEE Transactions on Speech and Audio Processing*, 8(3), 328-344. Hybrid monopole/dipole noise source models for realistic fricative synthesis.

### Web Resources

- [Klatt Synthesizer Parameters](https://linguistics.berkeley.edu/plab/guestwiki/index.php?title=Klatt_Synthesizer_Parameters) -- Berkeley Phonlab wiki with parameter descriptions and ranges
- [Acoustic Structure of Consonants](https://www.phon.ox.ac.uk/jcoleman/consonant_acoustics.htm) -- Oxford Phonetics, frequency data for all consonant categories
- [Acoustic Aspects of Consonants](https://corpus.eduhk.hk/english_pronunciation/index.php/3-2-acoustic-aspects-of-consonants/) -- EdUHK, VOT values and fricative frequency bands
- [Speech Synthesis: Fricative Consonants](https://charlesames.net/sound/speech-fricatives.html) -- Charles Ames, detailed filter chain specifications per fricative
- [Vokinesis: Syllabic Control Points](https://homes.create.aau.dk/dano/nime17/papers/0037/paper0037.pdf) -- NIME 2017, musical control of consonant-vowel timing
- [KlattGrid Speech Synthesizer](https://www.fon.hum.uva.nl/praat/manual/KlattGrid.html) -- Praat manual, modern Klatt implementation reference
- [Voice Onset Time](https://en.wikipedia.org/wiki/Voice_onset_time) -- Wikipedia, VOT overview with English stop values
- [Sibilance Frequencies](https://theproaudiofiles.com/vocal-sibilance/) -- Pro Audio Files, practical sibilance frequency ranges
- [Phonetics of Fricatives](https://kuppl.ku.edu/sites/kuppl/files/documents/publications/Jongman%20OREL%202024%20Phonetics%20of%20Fricatives.pdf) -- Jongman, comprehensive fricative spectral data
- [klatt-syn (GitHub)](https://github.com/chdh/klatt-syn) -- TypeScript Klatt implementation with amplitude parameter tables
