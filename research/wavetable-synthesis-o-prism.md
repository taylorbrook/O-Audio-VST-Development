# Research Findings: Wavetable Synthesis for O-Prism -- Microtonal Wavetable Synthesizer

## Domain
Wavetable Synthesis -- Factory Library Design, Procedural Generation Algorithms, Custom Import Pipeline, Spectral Resynthesis, Wavetable Editor Architecture, Modulation Matrix Architecture, and Microtonal-Specific Considerations.

---

## 1. Factory Wavetable Library Design (100+ Tables)

### 1.1 Category Organization

Based on analysis of Serum, Vital, Phase Plant, and Pigments, the recommended category structure for O-Prism with 100+ tables is:

**Category breakdown (112 tables total):**

| Category | Count | Description |
|----------|-------|-------------|
| Analog | 16 | Classic synth waveforms and morphs |
| Digital | 16 | FM, additive, bitcrushed, algorithmic |
| Spectral | 16 | Formant, comb, spectral tilt, resonant |
| Organic | 12 | Vocal, breath, string, woodwind-like |
| Metallic | 12 | Bells, bars, inharmonic, FM metallic |
| Textural | 12 | Noise-based, granular, evolving |
| Bass | 8 | Sub-optimized, thick fundamental tables |
| Leads | 8 | Bright, cutting, harmonically rich |
| Microtonal | 12 | JI-optimized, harmonic series, Bohlen-Pierce |

Serum organizes its factory content into four top-level folders: Analog, Digital, Spectral, and Vowel, with a User folder for custom content. Vital uses a similar but slightly different scheme. For O-Prism, expanding to nine categories with a Microtonal category (unique differentiator) provides better navigation for a 100+ library.

### 1.2 What Makes a "Good" Wavetable per Category

**Leads:** Bright harmonics, strong odd harmonics, spectral evolution from muted to searing. Multi-frame tables that morph from rounded to aggressive. Position 0 should be usable as a starting timbre, position 1.0 as the most extreme.

**Pads:** Slow spectral evolution across many frames (64-256 frames ideal). Smooth inter-frame transitions. Gentle harmonic content with subtle movement. Formant-based tables and spectral morphs work well.

**Bass:** Strong fundamental with controlled upper partials. Fewer frames needed (8-32). Emphasis on the first 8-12 harmonics. PWM-type tables add movement without losing low-end weight.

**Keys:** Quick harmonic decay characteristics baked into frame progression. Bell-like inharmonic partials. FM-derived timbres with moderate modulation index.

**Plucks:** Bright attack frames that darken quickly across the table. Emulate the natural spectral envelope of plucked strings.

**FX:** Extreme spectral morphs, noise-to-tone transitions, formant sweeps, granular-textured frames, reverse envelopes baked into frame order.

---

## 2. Procedural Wavetable Generation Algorithms

The existing `WavetableGenerator` in `/Users/taylorbrook/Dev/VST-development/plugins/O-Prism/Source/dsp/WavetableGenerator.cpp` generates Saw, Square, Triangle, and Sine via additive synthesis. The following algorithms extend this to produce the full 100+ library.

### 2.1 PWM (Pulse Width Modulation) Tables

PWM tables morph from a square wave to increasingly narrow pulse waves. This is a multi-frame table where each frame has a different duty cycle.

**Algorithm:**
```
For frame f in [0, numFrames-1]:
    dutyCycle = 0.5 - (f / (numFrames-1)) * 0.45  // 50% down to 5%
    For harmonic n in [1, maxHarmonics]:
        amplitude = (2 / (n * pi)) * sin(n * pi * dutyCycle)
        sample[i] += amplitude * sin(2*pi*n*i / tableSize)
```

**Harmonic recipe for a 50% duty cycle (standard square):**
- Partial 1: amplitude = 1.0
- Partial 3: amplitude = 1/3
- Partial 5: amplitude = 1/5
- (odd harmonics only)

**For 25% duty cycle (narrow pulse):**
- Partial 1: amplitude = sin(pi/4) * 2/pi = 0.450
- Partial 2: amplitude = sin(pi/2) * 1/pi = 0.318
- Partial 3: amplitude = sin(3pi/4) * 2/(3pi) = 0.150
- All harmonics present (not just odds)

**Implementation for O-Prism (32 frames, table "PWM Sweep"):**
```cpp
auto table = std::make_unique<WavetableData>();
table->allocate(32);
for (int frame = 0; frame < 32; ++frame) {
    float dutyCycle = 0.50f - frame * (0.45f / 31.0f);
    float* buf = table->getFrameData(0, frame);
    std::fill(buf, buf + WavetableData::kTableSize, 0.0f);
    int maxH = WavetableData::kTableSize / 2;
    for (int n = 1; n <= maxH; ++n) {
        double amp = (2.0 / (n * M_PI)) * std::sin(n * M_PI * dutyCycle);
        for (int i = 0; i < WavetableData::kTableSize; ++i)
            buf[i] += static_cast<float>(amp * std::sin(2.0 * M_PI * n * i / WavetableData::kTableSize));
    }
    // Normalize frame
    float maxVal = 0.0f;
    for (int i = 0; i < WavetableData::kTableSize; ++i)
        maxVal = std::max(maxVal, std::abs(buf[i]));
    if (maxVal > 0.0f)
        for (int i = 0; i < WavetableData::kTableSize; ++i)
            buf[i] /= maxVal;
}
WavetableGenerator::generateMipmaps(*table);
```

### 2.2 Formant/Vowel Tables

Formant synthesis creates vowel-like timbres by shaping the spectral envelope to match human vocal resonances. The key data comes from the Csound formant table (derived from the KLATT model):

**Formant Frequency Reference (Male Voice -- Tenor):**

| Vowel | F1 (Hz) | F2 (Hz) | F3 (Hz) | F4 (Hz) | F5 (Hz) |
|-------|---------|---------|---------|---------|---------|
| /a/ (father) | 650 | 1080 | 2650 | 2900 | 3250 |
| /e/ (bet) | 400 | 1700 | 2600 | 3200 | 3580 |
| /i/ (beet) | 290 | 1870 | 2800 | 3250 | 3540 |
| /o/ (boat) | 400 | 800 | 2600 | 2800 | 3000 |
| /u/ (boot) | 350 | 600 | 2700 | 2900 | 3300 |

**Formant Amplitudes (relative to F1, in dB):**

| Vowel | A1 | A2 | A3 | A4 | A5 |
|-------|-----|-----|------|------|------|
| /a/ | 0 | -6 | -7 | -8 | -22 |
| /e/ | 0 | -14 | -12 | -14 | -20 |
| /i/ | 0 | -15 | -9 | -20 | -28 |
| /o/ | 0 | -10 | -12 | -12 | -26 |
| /u/ | 0 | -20 | -17 | -14 | -26 |

**Bandwidths (Hz):**

| Vowel | BW1 | BW2 | BW3 | BW4 | BW5 |
|-------|------|------|------|------|------|
| /a/ | 80 | 90 | 120 | 130 | 140 |
| /e/ | 70 | 80 | 100 | 120 | 120 |
| /i/ | 40 | 90 | 100 | 120 | 120 |
| /o/ | 40 | 80 | 100 | 120 | 120 |
| /u/ | 40 | 60 | 100 | 120 | 120 |

**Algorithm -- Spectral Envelope Approach:**

Rather than using parallel resonators (KLATT style), for wavetable generation the efficient approach is to compute a spectral envelope from formant data and apply it to a harmonic series:

```
For each harmonic n (1 to maxHarmonics):
    freqOfHarmonic = n * fundamentalFreq  // Use reference pitch, e.g., 261.63 Hz (C4)
    spectralGain = 0.0
    For each formant f in [F1..F5]:
        distance = (freqOfHarmonic - formantFreq[f]) / bandwidth[f]
        resonance = formantAmp[f] * exp(-0.5 * distance * distance)
        spectralGain += resonance
    amplitude[n] = spectralGain
```

**Multi-frame vowel morph table (64 frames, "Vowel Sweep A-E-I-O-U"):**
- Frames 0-12: /a/ morphing to /e/
- Frames 13-25: /e/ morphing to /i/
- Frames 26-38: /i/ morphing to /o/
- Frames 39-51: /o/ morphing to /u/
- Frames 52-63: /u/ morphing back to /a/

Morphing is done by linear interpolation of formant frequencies, amplitudes, and bandwidths between adjacent vowels.

### 2.3 FM Synthesis-Derived Tables

FM synthesis generates rich spectra from simple carrier-modulator relationships. The fundamental formula is:

```
y(t) = sin(2*pi*fc*t + modulationIndex * sin(2*pi*fm*t))
```

Where:
- `fc` = carrier frequency (set to fundamental = 1.0 for single-cycle)
- `fm` = modulator frequency (ratio to carrier)
- `modulationIndex` = `Am / fm` (determines brightness/number of sidebands)

**Key FM Recipes:**

| Name | C:M Ratio | Index Range | Character |
|------|-----------|-------------|-----------|
| Electric Piano | 1:1 | 0.5-3.0 | DX7 Rhodes, warm |
| Brass | 1:1 | 1.0-8.0 | Bright, brassy |
| Bell | 1:1.4 | 3.0-10.0 | Metallic, inharmonic |
| Clarinet | 1:3 | 0.5-3.0 | Hollow, odd-harmonic |
| Metallic | 1:7 | 2.0-6.0 | Dense metallic |
| Organ | 1:2 | 0.5-4.0 | Rich, even harmonics |
| Clang | 1:1.414 | 5.0-15.0 | Harsh, bell-like |
| Tubular | 1:3.5 | 2.0-8.0 | Tube-like inharmonic |

**Implementation (16 frames per table, index sweep):**
```cpp
void generateFMTable(WavetableData& table, double cmRatio, 
                     double minIndex, double maxIndex, int numFrames) {
    table.allocate(numFrames);
    for (int frame = 0; frame < numFrames; ++frame) {
        double modIndex = minIndex + (maxIndex - minIndex) * frame / (numFrames - 1);
        float* buf = table.getFrameData(0, frame);
        for (int i = 0; i < WavetableData::kTableSize; ++i) {
            double t = static_cast<double>(i) / WavetableData::kTableSize;
            double modulator = modIndex * std::sin(2.0 * M_PI * cmRatio * t);
            buf[i] = static_cast<float>(std::sin(2.0 * M_PI * t + modulator));
        }
    }
    generateMipmaps(table);
}
```

**Important note:** FM-generated waveforms can contain harmonics above Nyquist. The existing mipmap generation via FFT truncation will handle this automatically -- the `generateMipmaps()` function already zeros bins above `maxHarmonic` per level.

### 2.4 Wavefolder-Derived Tables

Wavefolding is a nonlinear waveshaping technique where the waveform "folds" back on itself when it exceeds a threshold, creating rich harmonic content from simple inputs.

**Sine Wavefolder (simplest, cleanest):**
```
y = sin(gain * x)
```
As gain increases, the sine function wraps around, creating increasing harmonics. The 5th harmonic is dominant.

**Triangle Wavefolder (Serge-style):**
```
y = 4 * |((x * gain / 4 + 0.75) % 1.0) - 0.5| - 1.0
```
Triangle folders produce stronger high harmonics than sine folders, with potential for aliasing at extreme settings.

**Implementation (32 frames, drive sweep):**
```cpp
void generateWavefoldTable(WavetableData& table, int numFrames) {
    table.allocate(numFrames);
    for (int frame = 0; frame < numFrames; ++frame) {
        double gain = 1.0 + frame * (8.0 / (numFrames - 1)); // 1x to 9x
        float* buf = table.getFrameData(0, frame);
        for (int i = 0; i < WavetableData::kTableSize; ++i) {
            double phase = 2.0 * M_PI * i / WavetableData::kTableSize;
            double input = std::sin(phase); // Start with sine
            buf[i] = static_cast<float>(std::sin(gain * input));
        }
        // Normalize
        float maxVal = 0.0f;
        for (int i = 0; i < WavetableData::kTableSize; ++i)
            maxVal = std::max(maxVal, std::abs(buf[i]));
        if (maxVal > 0.0f)
            for (int i = 0; i < WavetableData::kTableSize; ++i)
                buf[i] /= maxVal;
    }
    generateMipmaps(table);
}
```

### 2.5 Supersaw / Detuned Stack Tables

The Roland JP-8000's SuperSaw is 7 detuned sawtooth oscillators. For wavetable "baking," the approach is to render the combined detuned oscillators into a single cycle and capture the resulting waveform.

**Algorithm:**
```
For each frame f (detune amount increases):
    detuneCents = f * (maxDetune / (numFrames - 1))
    For each of 7 voices v:
        detuneOffset = (v - 3) / 3.0 * detuneCents  // Spread ±detuneCents
        freqRatio = pow(2, detuneOffset / 1200.0)
        For each sample i:
            phase = frac(freqRatio * i / tableSize)
            accumulate bandlimited saw at this phase
    Normalize combined result
```

**Recommended tables:**
- "SuperSaw Spread" (32 frames: mono saw -> 7-voice max detune)
- "HyperSquare" (same with square waves)
- "Detuned Stack" (5 saws with asymmetric detune for motion)

**Note on anti-aliasing:** When baking detuned stacks, each constituent oscillator must be bandlimited before summation. Use the same additive harmonic generation as the existing `generateSaw()` but with fractional frequency ratios, limiting harmonics such that `n * freqRatio < tableSize / 2`.

### 2.6 Spectral Processing Tables

**Comb-Filtered Table (32 frames, comb frequency sweep):**
```
Start with broadband harmonics (saw or noise-like spectrum)
For each frame:
    Apply spectral comb filter: zero every N-th harmonic
    N varies from 2 to 16 across frames
```

**Spectral Tilt Table (16 frames):**
```
Start with all harmonics at equal amplitude
For each frame f:
    tiltAmount = -6 * f  // dB/octave, from 0 to -96 dB/oct
    amplitude[n] = pow(n, tiltAmount / (20 * log10(2)))
```
This sweeps from white-noise-like (flat spectrum) to sine-like (steep tilt).

**Harmonic Stretch Table (16 frames):**
```
For each frame f:
    stretchFactor = 1.0 + f * 0.1  // 1.0 to 2.5
    For harmonic n:
        actualFrequency = n^stretchFactor  // Stretched partial
        if (actualFrequency < tableSize/2):
            Place energy at nearest bin
```
Creates increasingly inharmonic timbres from harmonic ones.

### 2.7 Organ Stops and Pipe Models

Hammond organ drawbar harmonics map directly to additive synthesis:

| Drawbar | Footage | Harmonic | Interval |
|---------|---------|----------|----------|
| 1 | 16' | 1 (sub-fundamental) | Sub-octave |
| 2 | 5-1/3' | 3 | 5th above sub |
| 3 | 8' | 2 | Fundamental |
| 4 | 4' | 4 | Octave |
| 5 | 2-2/3' | 6 | Octave + 5th |
| 6 | 2' | 8 | 2 octaves |
| 7 | 1-3/5' | 10 | 2 octaves + 3rd |
| 8 | 1-1/3' | 12 | 2 octaves + 5th |
| 9 | 1' | 16 | 3 octaves |

**Classic registration recipes (drawbar values 0-8):**

| Name | Drawbars (1-9) | Character |
|------|---------------|-----------|
| Full Organ | 888888888 | Full, rich |
| Jazz | 838000000 | Warm, round |
| Gospel | 888643200 | Bright, churchy |
| Rock | 888800000 | Aggressive |
| Flute 8' | 008000000 | Pure fundamental |
| Strings | 004565300 | String-like |
| Clarinet | 007234100 | Hollow, reedy |
| Trumpet | 006788654 | Brassy |

**Implementation:**
```cpp
void generateOrganTable(WavetableData& table, int numFrames) {
    // 16 frames morphing through classic registrations
    const int drawbarHarmonics[] = {1, 3, 2, 4, 6, 8, 10, 12, 16};
    const float registrations[][9] = {
        {0,0,8,0,0,0,0,0,0}, // Pure 8'
        {0,0,8,4,0,0,0,0,0}, // Flute + 4th harmonic
        {0,0,8,8,0,0,0,0,0}, // Two footages
        {8,0,8,0,0,0,0,0,0}, // + Sub
        {8,3,8,0,0,0,0,0,0}, // + 5-1/3'
        // ... continue through registrations
        {8,8,8,8,8,8,8,8,8}, // Full organ
    };
    // For each frame, sum sine waves at drawbar harmonics with drawbar amplitudes
}
```

### 2.8 Inharmonic / Bell / Metallic Tones

Bell timbres use inharmonic partial ratios. Key reference ratios:

**Church Bell Partials (relative to strike note):**

| Partial | Name | Ratio | Cents |
|---------|------|-------|-------|
| 1 | Hum | 0.500 | -1200 |
| 2 | Prime | 1.000 | 0 |
| 3 | Tierce | 1.183 | +291 |
| 4 | Quint | 1.506 | +710 |
| 5 | Nominal | 2.000 | +1200 |
| 6 | Deciem | 2.514 | +1596 |
| 7 | Undeciem | 2.997 | +1898 |
| 8 | Duodeciem | 3.502 | +2117 |

**Tubular Bell Partials (modes 1-6):**

Frequencies follow the pattern: `f_n = f_1 * n^1.651` (approximately)
- Mode 1: 1.000
- Mode 2: 3.14
- Mode 3: 6.15
- Mode 4: 9.96 (these three determine strike pitch)
- Mode 5: 14.6
- Mode 6: 20.0

**Metal Bar Partials (free-free beam):**
`f_n = f_1 * (n + 0.5)^2 / 2.25`
- 1.000, 2.778, 5.444, 8.944, 13.333...

**Implementation note:** Since these are inharmonic, each partial may not land on an integer harmonic number. For wavetable generation, compute the exact sample position for each partial and use additive synthesis with non-integer partial frequencies:

```cpp
void generateBellTable(WavetableData& table, const double* partialRatios,
                       const double* partialAmps, int numPartials, int numFrames) {
    table.allocate(numFrames);
    for (int frame = 0; frame < numFrames; ++frame) {
        // Decay multiplier per frame (simulates bell decay envelope)
        double decayEnvelope = 1.0 - static_cast<double>(frame) / numFrames;
        float* buf = table.getFrameData(0, frame);
        std::fill(buf, buf + WavetableData::kTableSize, 0.0f);
        for (int p = 0; p < numPartials; ++p) {
            // Higher partials decay faster
            double partialDecay = std::pow(decayEnvelope, 1.0 + p * 0.3);
            double amp = partialAmps[p] * partialDecay;
            double ratio = partialRatios[p];
            for (int i = 0; i < WavetableData::kTableSize; ++i) {
                buf[i] += static_cast<float>(
                    amp * std::sin(2.0 * M_PI * ratio * i / WavetableData::kTableSize));
            }
        }
        // Normalize
        float maxVal = 0.0f;
        for (int i = 0; i < WavetableData::kTableSize; ++i)
            maxVal = std::max(maxVal, std::abs(buf[i]));
        if (maxVal > 0.0f)
            for (int i = 0; i < WavetableData::kTableSize; ++i)
                buf[i] /= maxVal;
    }
    generateMipmaps(table);
}
```

**Important:** Inharmonic partials will alias differently than harmonics during mipmap generation. The existing mipmap code zeros bins above `maxHarmonic`, which works for harmonic content but may remove energy for inharmonic partials that fall between integer bins. For bell/metallic tables, consider generating mipmaps by progressively lowpass-filtering the original waveform in the frequency domain, removing all spectral content above the Nyquist threshold for each mipmap level, rather than assuming partials are at integer multiples.

### 2.9 Noise-Based Textural Tables

**Filtered Noise Spectrum Table (32 frames):**
```
For each frame:
    Generate random phase for each bin
    Shape magnitude spectrum with parametric filter curve
    Frame 0: broadband white noise spectrum
    Frame 15: narrowband resonant peak (formant-like)
    Frame 31: very narrow, almost tonal
    IFFT to time domain
```

**Granular Texture Table (64 frames):**
Generate frames by randomly combining sine components at random phases, with a spectral envelope that evolves:
```
For each frame f:
    numActivePartials = 4 + f * 60 / 63  // 4 to 64 partials
    For p in [0, numActivePartials]:
        frequency = random_choice(1 to tableSize/4)
        amplitude = random(0.3, 1.0) / sqrt(numActivePartials)
        phase = random(0, 2*pi)
        Add to frame buffer
```

### 2.10 Complete Factory Library Specification

Here is the recommended 112-table library with specific generation parameters:

**Analog (16 tables):**
1. `Saw Classic` -- Standard bandlimited saw, 1 frame
2. `Square Classic` -- Standard bandlimited square, 1 frame
3. `Triangle Classic` -- Standard bandlimited triangle, 1 frame
4. `Sine Pure` -- Pure sine, 1 frame
5. `PWM Sweep` -- 50% to 5% duty cycle, 32 frames
6. `Saw->Square Morph` -- Odd harmonics fade in, 32 frames
7. `Saw->Triangle Morph` -- 1/n to 1/n^2 rolloff sweep, 32 frames
8. `Soft Saw` -- Saw with progressive harmonic rolloff, 16 frames
9. `Hard Sync Sweep` -- Simulated sync, slave ratio 1.0 to 4.0, 32 frames
10. `Sub Bass` -- Strong fundamental + octave, minimal upper harmonics, 8 frames
11. `Analog Stack 2` -- Two detuned saws baked, 16 frames
12. `Analog Stack 5` -- Five detuned saws baked, 16 frames
13. `Warm Pad` -- Saw with -12dB/oct rolloff + slight chorus baked, 16 frames
14. `Vintage Square` -- Square with even-harmonic leakage (analog imperfection), 8 frames
15. `Round Pulse` -- Pulse with softened edges (Gaussian windowed harmonics), 16 frames
16. `Phase Distortion` -- CZ-style cosine phase distortion, 32 frames

**Digital (16 tables):**
17. `FM Electric Piano` -- C:M=1:1, index 0.5-3.0, 16 frames
18. `FM Brass` -- C:M=1:1, index 1.0-8.0, 16 frames
19. `FM Bell` -- C:M=1:1.4, index 3.0-10.0, 32 frames
20. `FM Clarinet` -- C:M=1:3, index 0.5-3.0, 16 frames
21. `FM Metallic` -- C:M=1:7, index 2.0-6.0, 16 frames
22. `FM Organ` -- C:M=1:2, index 0.5-4.0, 16 frames
23. `Additive Bright` -- All harmonics, equal amplitude -> steep rolloff, 16 frames
24. `Additive Odds` -- Odd harmonics only, building up, 32 frames
25. `Bitcrush Sweep` -- Quantized amplitude levels 256 -> 4, 16 frames
26. `Digital Noise` -- Sample-and-hold at decreasing rates, 16 frames
27. `Sync Sweep Digital` -- Perfect hard sync simulation, 32 frames
28. `Ring Mod` -- Carrier * modulator ring modulation, 16 frames
29. `Wavefold Sine` -- Sine through fold, gain 1x-9x, 32 frames
30. `Wavefold Triangle` -- Triangle through fold, gain 1x-6x, 32 frames
31. `Phase Warp` -- Phase distortion a la Casio CZ, 32 frames
32. `Glitch` -- Random harmonic patterns per frame, 64 frames

**Spectral (16 tables):**
33. `Vowel A-E-I-O-U` -- Male formant morph, 64 frames
34. `Vowel Soprano` -- Female formant morph, 64 frames
35. `Formant Choir` -- Blended male+female formants, 64 frames
36. `Comb Filter Sweep` -- Comb spacing 2-16, 32 frames
37. `Spectral Tilt` -- 0 to -96 dB/oct tilt, 16 frames
38. `Resonant Peak Sweep` -- Moving resonant peak 200-8000Hz, 64 frames
39. `Harmonic Stretch` -- Stretch factor 1.0-2.5, 16 frames
40. `Bandpass Sweep` -- Narrow bandpass moving up spectrum, 64 frames
41. `Notch Sweep` -- Moving notch filter, 64 frames
42. `Spectral Blur` -- Progressive spectral smoothing, 16 frames
43. `Phase Scramble` -- Increasing random phase perturbation, 16 frames
44. `Spectral Freeze` -- Single frame captured spectra (varied), 8 frames
45. `Harmonic Series 1-32` -- First N harmonics, N increasing, 32 frames
46. `Subharmonic` -- Subharmonic series (dividing fundamental), 16 frames
47. `Prism Spectrum` -- Harmonics with rainbow-like spectral coloring, 32 frames
48. `Formant Filter` -- Parallel resonator bank, 32 frames

**Organic (12 tables):**
49. `Breath` -- Formant+noise blend, 32 frames
50. `Vocal Ah` -- Extended /a/ formant with vibrato baked, 64 frames
51. `Vocal Ooh` -- Extended /u/ formant, 32 frames
52. `String Bow` -- Sawtooth + formant resonances (body modes), 32 frames
53. `Plucked String` -- Bright->dark frame progression, 32 frames
54. `Woodwind` -- Odd harmonics + breath noise, 32 frames
55. `Flute` -- Near-sine with slight overblowing, 16 frames
56. `Reed` -- Square-ish + formant coloring, 16 frames
57. `Choir Pad` -- Blended vowel formants, slow morph, 64 frames
58. `Wind` -- Filtered noise with resonance, 16 frames
59. `Ocean` -- Very low frequency modulated noise spectrum, 32 frames
60. `Whisper` -- Formant-shaped noise, no pitch, 16 frames

**Metallic (12 tables):**
61. `Church Bell` -- Bell partial ratios, decay progression, 32 frames
62. `Tubular Bell` -- Tubular bell modes, 32 frames
63. `Metal Bar` -- Free beam partials, 16 frames
64. `Glockenspiel` -- Near-harmonic with slight stretch, 16 frames
65. `Vibraphone` -- Warm bell with tremolo baked, 32 frames
66. `Gamelan` -- Indonesian gamelan partial ratios, 16 frames
67. `FM Clang` -- C:M=1:1.414, high index, 32 frames
68. `Steel Drum` -- Characteristic partial structure, 16 frames
69. `Cymbal` -- Dense inharmonic partials, 8 frames
70. `Singing Bowl` -- Near-harmonic with beating, 32 frames
71. `Crystal` -- Very high inharmonic partials, sparse, 16 frames
72. `Industrial` -- Dense FM metallic noise, 16 frames

**Textural (12 tables):**
73. `White to Tone` -- Noise->pitched transition, 64 frames
74. `Grain Cloud` -- Random partial clusters, 64 frames
75. `Evolving Texture` -- Slowly changing random spectrum, 128 frames
76. `Static` -- Sample-and-hold noise at various rates, 16 frames
77. `Vinyl Crackle` -- Sparse impulse + LP filter, 16 frames
78. `Tape Hiss` -- Shaped high-frequency noise, 8 frames
79. `Underwater` -- Heavy LP filtered noise with resonance, 32 frames
80. `Electric Buzz` -- 60Hz harmonics (mains hum), 8 frames
81. `Dust` -- Sparse random impulses, 16 frames
82. `Ice` -- High-frequency crystalline texture, 16 frames
83. `Fire` -- Modulated noise with crackle, 32 frames
84. `Digital Artifacts` -- Quantization + aliasing textures, 16 frames

**Bass (8 tables):**
85. `Reese Bass` -- Two detuned saws, thick, 16 frames
86. `808 Sub` -- Sine + slight harmonics, 8 frames
87. `Acid Bass` -- Saw with resonant filter baked, 32 frames
88. `Wobble Bass` -- LFO-modulated spectrum baked, 64 frames
89. `Distorted Bass` -- Waveshaped saw, 16 frames
90. `Pluck Bass` -- Bright attack -> dark sustain, 32 frames
91. `FM Bass` -- C:M=1:1, low index, warm, 16 frames
92. `Growl Bass` -- Formant + wavefold combination, 32 frames

**Leads (8 tables):**
93. `Sync Lead` -- Hard sync sweep, bright, 32 frames
94. `Detuned Lead` -- 3 stacked saws, 16 frames
95. `FM Lead` -- C:M=1:1, medium index, 16 frames
96. `Scream Lead` -- Wavefold + resonance, aggressive, 32 frames
97. `Trance Lead` -- SuperSaw style, wide, 16 frames
98. `Moog Lead` -- Saw + LP filter at various cutoffs, 32 frames
99. `Glassy Lead` -- Triangle + slight FM, 16 frames
100. `Vocal Lead` -- Formant /e/-/a/ morph, 32 frames

**Microtonal (12 tables):**
101. `Harmonic Series 1-16` -- Pure harmonics, ideal for JI, 16 frames
102. `Harmonic Series 1-32` -- Extended harmonics, 32 frames
103. `7-Limit JI` -- Only partials from 7-limit ratios (1,3,5,7,9...), 16 frames
104. `Bohlen-Pierce` -- Partials at tritave ratios (odd harmonics only, no evens), 16 frames
105. `Just 5ths Stack` -- Stacked 3:2 ratios, 8 frames
106. `Just 3rds Stack` -- Stacked 5:4 ratios, 8 frames
107. `Septimal` -- 7th harmonic dominant timbre, 8 frames
108. `11-Limit` -- Including 11th harmonic prominence, 8 frames
109. `Combination Tones` -- Pairs of harmonics chosen for clear difference tones, 16 frames
110. `Gamelan Slendro` -- Indonesian scale-optimized partials, 16 frames
111. `Quarter-Tone Shimmer` -- Partials at 24-EDO intervals, 16 frames
112. `Spectral JI Morph` -- Morph from 12-TET-optimized to JI-optimized spectrum, 32 frames

---

## 3. Custom Wavetable Import Pipeline (WAV/AIFF to WavetableData)

### 3.1 Overview

The import pipeline converts an arbitrary WAV/AIFF file into a `WavetableData` structure. There are three import modes, matching Serum and Vital conventions:

1. **Fixed-length slicing** -- Split every N samples (default: 2048)
2. **Pitch-detection slicing** -- Detect fundamental period and extract single cycles
3. **Serum-compatible import** -- Read the `clm` metadata chunk

### 3.2 File Loading with JUCE

```cpp
#include <JuceHeader.h>

struct ImportedAudio {
    juce::AudioBuffer<float> buffer;
    double sampleRate;
    int numChannels;
    juce::int64 numSamples;
};

std::optional<ImportedAudio> loadAudioFile(const juce::File& file) {
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats(); // WAV, AIFF, FLAC, Ogg
    
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));
    
    if (reader == nullptr)
        return std::nullopt;
    
    ImportedAudio result;
    result.sampleRate = reader->sampleRate;
    result.numChannels = static_cast<int>(reader->numChannels);
    result.numSamples = reader->lengthInSamples;
    result.buffer.setSize(result.numChannels, 
                          static_cast<int>(result.numSamples));
    reader->read(&result.buffer, 0, static_cast<int>(result.numSamples), 
                 0, true, true);
    
    return result;
}
```

### 3.3 Serum CLM Metadata Chunk

The CLM chunk is a custom RIFF chunk in WAV files that specifies wavetable metadata:

**Format:**
```
Chunk ID:   'clm ' (4 bytes: 0x63 0x6C 0x6D 0x20)
Chunk Size: (4 bytes, little-endian)
Data:       ASCII string: "<!>AAAA BC000000 D"
```

Where:
- `AAAA` = Cycle size (almost always `2048`)
- `B` = Interpolation type: 0=None, 1=Linear crossfade, 2/3/4=Spectral morph
- `C` = Serum factory flag (0 for user content, 1 for Serum factory)
- `D` = Vendor string / comment

**Example string:** `<!>2048 01000000 wavetable (www.xferrecords.com)`

**Vital compatibility note:** Vital interprets B=1 as "Time" blend, and B=2/3/4 as "Spectral" blend. The chunk size must be even for Vital to read it correctly.

**Reading the CLM chunk in JUCE:**

```cpp
struct CLMMetadata {
    int cycleSize = 2048;
    int interpolationType = 0;
    bool isFactory = false;
    juce::String vendorString;
    bool found = false;
};

CLMMetadata readCLMChunk(const juce::File& wavFile) {
    CLMMetadata result;
    juce::FileInputStream fis(wavFile);
    if (!fis.openedOk()) return result;
    
    // Read RIFF header
    char riffId[4];
    fis.read(riffId, 4);
    if (std::memcmp(riffId, "RIFF", 4) != 0) return result;
    
    fis.readInt(); // File size
    char waveId[4];
    fis.read(waveId, 4);
    if (std::memcmp(waveId, "WAVE", 4) != 0) return result;
    
    // Scan chunks looking for 'clm '
    while (!fis.isExhausted()) {
        char chunkId[4];
        fis.read(chunkId, 4);
        int chunkSize = fis.readInt();
        
        if (std::memcmp(chunkId, "clm ", 4) == 0) {
            // Read the ASCII data
            juce::MemoryBlock block(static_cast<size_t>(chunkSize));
            fis.read(block.getData(), chunkSize);
            juce::String clmString(static_cast<const char*>(block.getData()), 
                                   static_cast<size_t>(chunkSize));
            
            // Parse: "<!>2048 01000000 wavetable (...)"
            if (clmString.startsWith("<!>")) {
                auto afterMarker = clmString.substring(3).trim();
                auto cycleStr = afterMarker.upToFirstOccurrenceOf(" ", false, false);
                result.cycleSize = cycleStr.getIntValue();
                
                auto remainder = afterMarker.fromFirstOccurrenceOf(" ", false, false).trim();
                if (remainder.length() >= 8) {
                    result.interpolationType = remainder[0] - '0';
                    result.isFactory = (remainder[1] == '1');
                    result.vendorString = remainder.substring(9).trim();
                }
                result.found = true;
            }
            break;
        }
        
        // Skip to next chunk (pad to even boundary)
        fis.setPosition(fis.getPosition() + ((chunkSize + 1) & ~1));
    }
    
    return result;
}
```

### 3.4 Fixed-Length Frame Slicing

The simplest approach. Assumes the audio is already organized as concatenated single-cycle frames.

```cpp
std::unique_ptr<WavetableData> importFixedLength(
    const juce::AudioBuffer<float>& source, double sourceSampleRate,
    int frameSize = WavetableData::kTableSize) 
{
    // Mix to mono if stereo
    juce::AudioBuffer<float> mono(1, source.getNumSamples());
    mono.clear();
    for (int ch = 0; ch < source.getNumChannels(); ++ch)
        mono.addFrom(0, 0, source, ch, 0, source.getNumSamples(), 
                     1.0f / source.getNumChannels());
    
    int totalSamples = mono.getNumSamples();
    int numFrames = std::min(totalSamples / frameSize, WavetableData::kMaxFrames);
    
    if (numFrames < 1) return nullptr;
    
    auto table = std::make_unique<WavetableData>();
    table->allocate(numFrames);
    
    for (int frame = 0; frame < numFrames; ++frame) {
        const float* src = mono.getReadPointer(0) + frame * frameSize;
        float* dest = table->getFrameData(0, frame);
        
        if (frameSize == WavetableData::kTableSize) {
            // Direct copy
            std::copy(src, src + WavetableData::kTableSize, dest);
        } else {
            // Resample to 2048 using linear interpolation
            double ratio = static_cast<double>(frameSize) / WavetableData::kTableSize;
            for (int i = 0; i < WavetableData::kTableSize; ++i) {
                double srcPos = i * ratio;
                int idx = static_cast<int>(srcPos);
                double frac = srcPos - idx;
                int idx1 = std::min(idx + 1, frameSize - 1);
                dest[i] = static_cast<float>(src[idx] * (1.0 - frac) + src[idx1] * frac);
            }
        }
    }
    
    WavetableGenerator::generateMipmaps(*table);
    return table;
}
```

### 3.5 Pitch-Detection-Based Slicing

For importing single-note recordings where cycle length varies. Uses autocorrelation for pitch detection.

```cpp
int detectPeriodSamples(const float* buffer, int numSamples, double sampleRate) {
    // Autocorrelation-based pitch detection
    // Search range: 20Hz to 4000Hz
    int minLag = static_cast<int>(sampleRate / 4000.0);
    int maxLag = std::min(static_cast<int>(sampleRate / 20.0), numSamples / 2);
    
    double bestCorrelation = -1.0;
    int bestLag = minLag;
    
    int analysisLength = maxLag * 2;
    
    for (int lag = minLag; lag <= maxLag; ++lag) {
        double correlation = 0.0;
        double energy1 = 0.0, energy2 = 0.0;
        
        for (int i = 0; i < analysisLength - lag; ++i) {
            correlation += buffer[i] * buffer[i + lag];
            energy1 += buffer[i] * buffer[i];
            energy2 += buffer[i + lag] * buffer[i + lag];
        }
        
        double normalizedCorrelation = correlation / 
            std::max(1e-10, std::sqrt(energy1 * energy2));
        
        if (normalizedCorrelation > bestCorrelation) {
            bestCorrelation = normalizedCorrelation;
            bestLag = lag;
        }
    }
    
    return (bestCorrelation > 0.8) ? bestLag : -1; // -1 = no clear pitch
}

std::unique_ptr<WavetableData> importPitchDetected(
    const juce::AudioBuffer<float>& source, double sampleRate) 
{
    const float* mono = source.getReadPointer(0);
    int totalSamples = source.getNumSamples();
    
    // Detect pitch from early portion of audio
    int period = detectPeriodSamples(mono, std::min(totalSamples, 8192), sampleRate);
    if (period <= 0) return nullptr;
    
    int numFrames = std::min(totalSamples / period, WavetableData::kMaxFrames);
    if (numFrames < 1) return nullptr;
    
    auto table = std::make_unique<WavetableData>();
    table->allocate(numFrames);
    
    for (int frame = 0; frame < numFrames; ++frame) {
        const float* cycleStart = mono + frame * period;
        float* dest = table->getFrameData(0, frame);
        
        // Resample from detected period to 2048
        double ratio = static_cast<double>(period) / WavetableData::kTableSize;
        for (int i = 0; i < WavetableData::kTableSize; ++i) {
            double srcPos = i * ratio;
            int idx = static_cast<int>(srcPos);
            double frac = srcPos - idx;
            int idx1 = std::min(idx + 1, period - 1);
            dest[i] = static_cast<float>(
                cycleStart[idx] * (1.0 - frac) + cycleStart[idx1] * frac);
        }
    }
    
    WavetableGenerator::generateMipmaps(*table);
    return table;
}
```

### 3.6 Serum-Compatible Import

Combines CLM metadata reading with fixed-length slicing:

```cpp
std::unique_ptr<WavetableData> importSerumWavetable(const juce::File& wavFile) {
    // 1. Try to read CLM metadata
    auto clm = readCLMChunk(wavFile);
    
    // 2. Load audio data
    auto audio = loadAudioFile(wavFile);
    if (!audio) return nullptr;
    
    int frameSize = clm.found ? clm.cycleSize : WavetableData::kTableSize;
    
    // 3. Slice using the frame size from metadata (or default 2048)
    return importFixedLength(audio->buffer, audio->sampleRate, frameSize);
}
```

### 3.7 Resampling Considerations

When source sample rate differs from the expected rate, or frame sizes do not evenly divide:

**High-quality resampling for import (non-realtime, can use sinc):**

```cpp
void resampleFrame(const float* src, int srcLen, float* dest, int destLen) {
    // Windowed sinc interpolation (high quality for offline use)
    constexpr int sincTaps = 16; // 16-tap sinc for quality
    double ratio = static_cast<double>(srcLen) / destLen;
    
    for (int i = 0; i < destLen; ++i) {
        double srcPos = i * ratio;
        int center = static_cast<int>(srcPos);
        double frac = srcPos - center;
        
        double sum = 0.0;
        double normalization = 0.0;
        
        for (int tap = -sincTaps; tap <= sincTaps; ++tap) {
            int srcIdx = center + tap;
            if (srcIdx < 0) srcIdx += srcLen; // Wrap (wavetable is periodic)
            if (srcIdx >= srcLen) srcIdx -= srcLen;
            
            double x = tap - frac;
            double sincVal = (std::abs(x) < 1e-10) ? 1.0 : 
                std::sin(M_PI * x) / (M_PI * x);
            // Blackman-Harris window
            double window = 0.35875 - 0.48829 * std::cos(2 * M_PI * (tap + sincTaps) / (2 * sincTaps))
                          + 0.14128 * std::cos(4 * M_PI * (tap + sincTaps) / (2 * sincTaps))
                          - 0.01168 * std::cos(6 * M_PI * (tap + sincTaps) / (2 * sincTaps));
            
            double weight = sincVal * window;
            sum += src[srcIdx] * weight;
            normalization += weight;
        }
        
        dest[i] = static_cast<float>(sum / std::max(normalization, 1e-10));
    }
}
```

### 3.8 Automatic Mipmap Generation from Imported Frames

The existing `WavetableGenerator::generateMipmaps()` in the codebase handles this correctly. It takes the level-0 (full-bandwidth) frame data, performs an FFT, progressively removes harmonics above the threshold for each mipmap level, and stores the IFFT result. This works for both procedurally generated and imported wavetables without modification.

---

## 4. Spectral Resynthesis from Audio

### 4.1 STFT-Based Frame Extraction

To create a wavetable from arbitrary audio (vocal recording, instrument sample, field recording), use the Short-Time Fourier Transform to capture spectral evolution:

```cpp
std::unique_ptr<WavetableData> spectralResynthesis(
    const juce::AudioBuffer<float>& source, double sampleRate,
    int numOutputFrames = 64) 
{
    constexpr int fftOrder = 11; // 2048-point FFT
    constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft(fftOrder);
    
    const float* audio = source.getReadPointer(0);
    int totalSamples = source.getNumSamples();
    
    // Calculate hop size to cover the full audio in numOutputFrames windows
    int hopSize = std::max(1, (totalSamples - fftSize) / (numOutputFrames - 1));
    
    auto table = std::make_unique<WavetableData>();
    table->allocate(numOutputFrames);
    
    std::vector<float> fftBuffer(fftSize * 2, 0.0f);
    std::vector<float> window(fftSize);
    
    // Hann window
    for (int i = 0; i < fftSize; ++i)
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (fftSize - 1)));
    
    for (int frame = 0; frame < numOutputFrames; ++frame) {
        int startSample = std::min(frame * hopSize, totalSamples - fftSize);
        if (startSample < 0) startSample = 0;
        
        // Window the audio segment
        std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
        for (int i = 0; i < fftSize && (startSample + i) < totalSamples; ++i)
            fftBuffer[i] = audio[startSample + i] * window[i];
        
        // Forward FFT
        fft.performRealOnlyForwardTransform(fftBuffer.data(), false);
        
        // Extract magnitudes, discard phase (or use minimum-phase reconstruction)
        std::vector<float> magnitudes(fftSize / 2 + 1);
        for (int bin = 0; bin <= fftSize / 2; ++bin) {
            float re = fftBuffer[bin * 2];
            float im = fftBuffer[bin * 2 + 1];
            magnitudes[bin] = std::sqrt(re * re + im * im);
        }
        
        // Reconstruct with minimum-phase: zero DC, assign minimum-phase 
        // (this makes the resulting single-cycle waveform smoother)
        std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
        fftBuffer[0] = 0.0f; // Zero DC
        fftBuffer[1] = 0.0f;
        for (int bin = 1; bin <= fftSize / 2; ++bin) {
            // Minimum phase: phase = -Hilbert(log(magnitude))
            // Simplified: use zero phase for practicality
            fftBuffer[bin * 2] = magnitudes[bin];     // Real = magnitude
            fftBuffer[bin * 2 + 1] = 0.0f;            // Imag = 0 (zero phase)
        }
        
        // IFFT
        fft.performRealOnlyInverseTransform(fftBuffer.data());
        
        // Store in table
        float* dest = table->getFrameData(0, frame);
        std::copy(fftBuffer.begin(), fftBuffer.begin() + fftSize, dest);
        
        // Normalize frame
        float maxVal = 0.0f;
        for (int i = 0; i < fftSize; ++i)
            maxVal = std::max(maxVal, std::abs(dest[i]));
        if (maxVal > 0.0f)
            for (int i = 0; i < fftSize; ++i)
                dest[i] /= maxVal;
    }
    
    WavetableGenerator::generateMipmaps(*table);
    return table;
}
```

### 4.2 Pitch-Synchronous Spectral Extraction

For pitched audio, extracting pitch-aligned cycles produces cleaner results:

1. Run pitch detection across the audio to find fundamental period at each point
2. Extract one cycle per detected period
3. Resample each extracted cycle to 2048 samples
4. Apply FFT, keep magnitudes, reconstruct with consistent phase

This avoids the smearing that occurs when STFT windows do not align with the fundamental period.

### 4.3 Spectral Envelope Extraction (Vocal Capture)

For capturing the timbral character of a voice without its pitch:

1. FFT the audio segment
2. Find spectral peaks (formants) using peak-picking or true envelope estimation
3. Fit a smooth spectral envelope through the peaks (e.g., using cepstral smoothing: FFT -> log -> IFFT -> low-pass lifter -> FFT -> exp -> IFFT)
4. Apply this envelope to a harmonic excitation signal (like a saw wave's harmonic series)
5. Store the result as a wavetable frame

This lets users import a vocal recording and get a wavetable that morphs through the vocal's spectral characteristics while being playable at any pitch.

---

## 5. Wavetable Editor Architecture (v2.0)

### 5.1 Editor Modes

Based on analysis of Serum, Vital, and Phase Plant editors, the recommended editor architecture has four primary modes:

**A. Additive Editor (Harmonic Sliders)**
- Display: Vertical bars for each harmonic (1-128 or 1-256)
- Two rows: amplitude bars (top) and phase bars (bottom)
- User drags bar heights to set harmonic amplitudes
- Phase display: 0 to 2*pi per harmonic
- Keyframe-based: edit harmonics per keyframe, interpolate between them
- Reconstruction: IFFT from harmonic data to time-domain waveform

**B. Spectral Editor (Frequency-Domain Drawing)**
- Display: Full spectral magnitude view (like a spectrogram slice)
- Freehand drawing tools for painting spectral magnitude
- Brush size controls how many bins are affected per stroke
- "Power Scale" option for logarithmic vs linear amplitude display
- Applies directly to FFT magnitude bins
- Reconstruction via IFFT

**C. Waveform Drawing Editor (Time-Domain)**
- Display: Single-cycle waveform view
- Freehand pencil tool for direct waveform drawing
- Spline/Bezier curve tool for smooth waveforms (breakpoint editing like Vital's Line Source)
- Grid snapping options (8, 16, 32, 64 divisions)
- "Pull Power" control (from Vital) determining how strongly the drawn shape affects harmonics

**D. Morphing Tools**
- **Linear Morph:** Time-domain sample-by-sample interpolation between keyframes
- **Spectral Morph:** Interpolation in frequency domain (FFT magnitudes and phases independently blended). Higher quality, avoids amplitude artifacts but computationally heavier.
- **Crossfade Morph:** Overlap-add between adjacent keyframes with configurable crossfade window
- **Smooth:** Gaussian smoothing applied to the spectral evolution across frames

### 5.2 Editor Data Model

```cpp
struct WavetableEditorState {
    struct Keyframe {
        int framePosition;  // 0-255
        
        // Additive representation (source of truth)
        std::array<float, 1024> harmonicAmplitudes {};
        std::array<float, 1024> harmonicPhases {};
        
        // Cached time-domain (regenerated from harmonics)
        std::array<float, 2048> waveformCache {};
        bool cacheValid = false;
    };
    
    std::vector<Keyframe> keyframes;
    
    enum class InterpolationMode { Linear, Spectral, Crossfade };
    InterpolationMode morphMode = InterpolationMode::Spectral;
    
    // Reconstruct full WavetableData from editor state
    std::unique_ptr<WavetableData> renderToWavetable(int numFrames = 256) const;
};
```

### 5.3 Real-Time Preview Architecture

The editor must support real-time audio preview while the user edits. The architecture:

1. **Editor thread** (UI/message thread): Modifies `WavetableEditorState` keyframes
2. **Render thread** (background): When keyframes change, renders full `WavetableData` in background
3. **Audio thread**: Reads from current `WavetableData*` via atomic pointer swap
4. **Preview oscillator**: Standalone `WavetableOscillator` instance playing a sustained note

The atomic pointer swap pattern already used in the codebase (see `updateWavetableAssignments()` in `/Users/taylorbrook/Dev/VST-development/plugins/O-Prism/Source/PluginProcessor.cpp` line 611) extends naturally to editor preview.

### 5.4 Modifier Pipeline (Post-Source Processing)

Following the Phase Plant model, the editor should support stackable modifiers applied after the source waveform:

| Modifier | Parameters | Effect |
|----------|------------|--------|
| Phase Shift | Amount, Mode (all/even/odd) | Rotates phase of partials |
| Wave Window | Shape, Left/Right position | Applies amplitude window |
| Frequency Filter | Type (LP/HP/BP/Comb), Cutoff | Spectral filtering |
| Slew Limiter | Up rate, Down rate | Smooths steep transitions |
| Wave Folder | Gain (1x-32x) | Nonlinear folding |
| Wave Warp | X-warp, Y-warp, Asymmetric | Time/amplitude reshaping |
| Harmonic Shift | Amount (semitones) | Pitch-shifts spectral content |

Each modifier is applied per-frame, and the full modifier chain is re-evaluated when any parameter changes.

---

## 6. Modulation Matrix Architecture (v2.0)

### 6.1 Core Data Structure

Based on analysis of Serum, Vital, and Phase Plant implementations:

```cpp
struct ModulationRoute {
    enum class Source {
        None = 0,
        LFO1, LFO2, LFO3, LFO4,      // 4 LFOs
        Env1, Env2, Env3,               // 3 extra envelopes
        AmpEnv, FiltEnv,                // Existing envelopes as sources
        ModWheel, Aftertouch,           // MIDI CC sources
        Velocity, KeyTrack,             // Per-note sources
        PitchBend,
        Random, StepSeq,                // Utility sources
        Macro1, Macro2, Macro3, Macro4  // Macro knobs
    };
    
    Source source = Source::None;
    int destinationParamIndex = -1;  // Index into APVTS parameter list
    float depth = 0.0f;             // -1.0 to +1.0 (bipolar)
    bool bipolar = false;           // Source polarity
    
    // Optional: secondary modulation of the depth
    Source depthModSource = Source::None;
    float depthModAmount = 0.0f;
};

class ModulationMatrix {
public:
    static constexpr int kMaxRoutes = 32;
    
    void addRoute(ModulationRoute route);
    void removeRoute(int index);
    
    // Called once per audio block to update all modulation values
    void processBlock(int numSamples);
    
    // Get the total modulation offset for a parameter
    float getModulationValue(int paramIndex) const;
    
private:
    std::array<ModulationRoute, kMaxRoutes> routes;
    int numActiveRoutes = 0;
    
    // Per-voice modulation state
    struct VoiceModState {
        std::array<float, kMaxRoutes> routeValues {};
    };
    
    // LFO and envelope generators
    struct LFOState {
        float phase = 0.0f;
        float rate = 1.0f;
        int shape = 0; // Sine, Triangle, Saw, Square, S&H, Custom
        float getNextSample(double sampleRate);
    };
    
    std::array<LFOState, 4> lfos;
    std::array<juce::ADSR, 3> extraEnvelopes;
    
    // Accumulated modulation per parameter
    std::unordered_map<int, float> paramModValues;
};
```

### 6.2 Processing Flow

For each audio block:

```
1. Update all modulation sources (LFOs advance, envelopes advance)
2. For each active route:
   a. Read source value (-1.0 to +1.0 normalized)
   b. Multiply by route depth
   c. If depth-mod source active, multiply depth by depth-mod value
   d. Accumulate into destination parameter's mod value
3. When reading parameters in voice processing:
   paramValue = apvtsValue + modulationMatrix.getModulationValue(paramIndex)
   Clamp to parameter's valid range
```

### 6.3 Per-Voice vs Global Modulation

- **Per-voice sources:** Velocity, KeyTrack, PitchBend, AmpEnv, FiltEnv, per-voice Random
- **Global sources:** LFOs, Macro knobs, ModWheel, Aftertouch, StepSeq
- **Per-voice modulation** requires evaluating the matrix within each voice's `renderNextBlock()`
- **Global modulation** can be evaluated once per block in `processBlock()`

### 6.4 UI Implementation (Drag-and-Drop)

In the WebView UI:

1. Modulation sources (LFO, Env icons) are draggable elements
2. All parameter knobs are drop targets
3. On drop, a modulation "ring" appears around the target knob showing mod depth
4. The ring color matches the source color (e.g., LFO1=blue, LFO2=green)
5. Dragging the ring adjusts modulation depth
6. A Matrix view shows all active routes in a table for overview editing
7. Right-click any modulation ring for depth amount, polarity toggle, and curve editor

### 6.5 LFO Shapes

The modulation LFOs should support:
- Sine, Triangle, Saw (up/down), Square, Sample-and-Hold
- Custom shape (drawn with breakpoint editor, similar to Vital)
- Rate: 0.01 Hz to 50 Hz (free) or tempo-synced (1/32 to 8 bars)
- Phase offset: 0-360 degrees
- Fade-in time: 0-10 seconds

---

## 7. Microtonal-Specific Wavetable Considerations

### 7.1 Mipmap Level Selection for Non-Octave Scales

The current mipmap implementation in `/Users/taylorbrook/Dev/VST-development/plugins/O-Prism/Source/dsp/WavetableOscillator.cpp` (line 94-98) calculates the mipmap level using:

```cpp
int WavetableOscillator::calculateMipmapLevel() const {
    double baseFreq = currentSampleRate / static_cast<double>(WavetableData::kTableSize);
    double levelFloat = std::log2(frequency / baseFreq);
    return juce::jlimit(0, WavetableData::kNumMipmapLevels - 1, static_cast<int>(levelFloat));
}
```

This uses `log2`, which inherently assumes octave doubling. For standard 12-TET and most tunings that repeat at the octave, this is correct. However, for non-octave-repeating scales like Bohlen-Pierce (which repeats at the tritave, ratio 3:1), the mipmap selection itself still works correctly because the mipmap level selection is based on the absolute frequency, not the scale structure. The harmonic content cutoff at each level is determined by the playback frequency relative to Nyquist, regardless of what tuning system produced that frequency.

**Conclusion:** The current mipmap implementation requires no changes for microtonal support. The frequency-based level selection is tuning-agnostic.

### 7.2 Anti-Aliasing Edge Cases

Where microtonal tuning does create issues:

**Non-standard intervals near octave boundaries:** In 12-TET, the octave is exactly 2:1, so moving up 12 semitones exactly doubles the frequency and shifts the mipmap level by exactly 1. In non-octave tunings, the "octave equivalent" interval may be larger or smaller:
- Bohlen-Pierce tritave: 3:1 (frequency triples, mipmap level shifts by ~1.585)
- Carlos Alpha: 78 cent steps, period at 1560 cents (ratio ~2.464)

The trilinear interpolation already in `readSample()` handles fractional mipmap levels smoothly, so this is not a problem in practice.

**Very high microtonal notes:** Some microtonal scales map many more than 12 notes per octave (e.g., 53-EDO has 53 notes per octave). Playing high on the keyboard in these scales means very high frequencies where mipmap levels approach the maximum. The existing 10 mipmap levels (covering a range of 2048/2^10 = 2 harmonics at the highest level) provides adequate coverage up to approximately 10.7 kHz fundamental at 44.1 kHz sample rate, which covers the MIDI range even in extreme tunings.

### 7.3 Designing Wavetables Optimized for Just Intonation

In just intonation, intervals are exact frequency ratios. Wavetables designed for JI should have harmonic content that reinforces these ratios rather than creating beating:

**Principle:** When two notes in a JI chord share harmonics (e.g., a 3:2 fifth where the 3rd harmonic of the lower note equals the 2nd harmonic of the upper note), those shared harmonics reinforce rather than beat. Wavetables that emphasize these shared harmonics sound cleaner in JI.

**7-Limit JI Optimized Table:**
Only include partials at positions that correspond to 7-limit ratios: 1, 3, 5, 7, 9, 15, 21, 35...
```
Partials: [1, 3, 5, 7, 9, 15, 21, 35, 45, 63, 105]
Amplitudes: [1.0, 0.5, 0.4, 0.35, 0.25, 0.15, 0.12, 0.08, 0.06, 0.04, 0.02]
```
These partials will create pure beating-free harmonies in 7-limit JI tunings.

**Bohlen-Pierce Optimized Table:**
Bohlen-Pierce uses a tritave (3:1) instead of octave (2:1) and is built on odd harmonics only. Wavetables for BP should exclude even harmonics entirely:
```
Partials: [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27]
Amplitudes: [1.0, 0.7, 0.5, 0.4, 0.3, 0.25, 0.2, 0.17, 0.14, 0.12, 0.1, 0.08, 0.07, 0.06]
```
Even harmonics (2, 4, 6, 8...) are explicitly excluded because they do not fit the Bohlen-Pierce harmonic framework.

### 7.4 Spectral Content for Microtonal Chord Voicings

**Beating and Combination Tones:**
When two oscillators play a microtonal interval, combination tones arise at frequencies f1 + f2 and f1 - f2 (and higher-order combinations). In JI, these combination tones often reinforce scale degrees. In tempered tunings, they can create audible beating.

**Recommendation:** The Microtonal category tables should be designed with:
1. **Sparse harmonics** (fewer partials = fewer beating opportunities)
2. **Strong fundamentals** (the 1st partial dominates, reducing harmonic clash)
3. **Rapid harmonic rolloff** (like -12 dB/octave or steeper)
4. **"Harmonic Series" tables** where partials follow actual harmonic series ratios, ideal for JI chords

### 7.5 Non-Octave Repetition and Mipmap Strategy

For non-octave-repeating scales, the "equivalent of an octave" in the scale may be a tritave (3:1), a 5th (3:2), or any other interval. The mipmap system does not need to know about this -- it operates purely on absolute frequency. However, for the wavetable editor (v2.0), the harmonic slider display could optionally show partials labeled by their relationship to the active tuning system rather than strict integer harmonics.

---

## 8. Recommendations Summary

### Near-Term (v1.x): Expanded Factory Library

1. **Implement a `WavetableFactory` class** with static methods for each generation algorithm (FM, wavefold, formant, organ, bell, PWM, supersaw, spectral). Each method returns `std::unique_ptr<WavetableData>`.

2. **Generate all 112 tables at startup or on first launch**, then cache to disk in a binary format. At ~20MB per full table (10 mipmap levels * 256 frames * 2049 samples * 4 bytes), 112 tables would require ~2.2 GB uncompressed. Recommend: use lazy loading (generate mipmaps on-demand) and reduce to single-frame or 16-32 frame tables where full 256 frames are unnecessary.

3. **Extend the `oscATable` / `oscBTable` parameter range** from `0-3` to `0-111` (or use a string-based table selector rather than an integer parameter).

4. **Memory optimization:** Store factory tables compressed and decompress on demand. Only keep the currently selected tables + adjacent tables in RAM for quick switching.

### Near-Term (v1.x): Custom Import

5. **Add WAV/AIFF import** using the pipeline described in Section 3. Support three modes: Auto (try CLM, fallback to fixed-length), Pitch-Detect, and Manual Frame Size.

6. **Write CLM metadata** when exporting wavetables to maintain Serum compatibility.

7. **Add spectral resynthesis** as an import option for arbitrary audio.

### Mid-Term (v2.0): Editor and Mod Matrix

8. **Wavetable editor** with additive, spectral, and waveform drawing modes, implemented in the WebView UI using Canvas/WebGL. The frequency-domain (additive/spectral) representation should be the source of truth, with time-domain cached from it.

9. **Modulation matrix** with 32 route slots, drag-and-drop UI, per-voice evaluation for note-level sources, global evaluation for LFOs/macros. Start with LFO x4, extra Env x3, MIDI sources, then add step sequencer and custom LFO shapes.

---

## JUCE Modules Needed

- `juce::dsp::FFT` -- Already in use for mipmap generation; also needed for spectral resynthesis and import
- `juce::AudioFormatManager` + `juce::WavAudioFormat` -- For WAV/AIFF import pipeline
- `juce::AudioFormatReader` -- Reading imported audio files
- `juce::AudioBuffer<float>` -- Holding imported audio data
- `juce::ADSR` -- Already in use; needed for additional mod matrix envelopes
- `juce::dsp::Oscillator` -- Could be used for LFO generation in mod matrix (or custom implementation)
- `juce::FileInputStream` -- For reading CLM chunks from WAV files
- `juce::MemoryBlock` -- For handling raw chunk data

All required modules are already linked in the current O-Prism build. No additional JUCE modules are needed.

---

## Confidence Level

**HIGH** -- The wavetable synthesis domain is well-documented academically and by the professional synth community. The algorithms described (additive synthesis, FM, wavefolding, formant shaping, mipmap anti-aliasing) are well-established techniques. The existing O-Prism architecture (2048-sample frames, 10 mipmap levels, FFT-based mipmap generation, trilinear interpolation) is already a solid foundation that directly supports all recommended extensions. The Serum CLM format is reverse-engineered and publicly documented. The microtonal considerations are based on first-principles analysis of the existing tuning engine architecture.

---

## Professional References

- **Xfer Serum:** Factory library organization (Analog/Digital/Spectral/Vowel categories), CLM metadata chunk format for wavetable interchange, 2048 sample/frame industry standard
- **Vital (Matt Tytel):** Wavetable editor architecture (Wave Source, Line Source, Audio File Source), spectral vs time-domain blend modes, modifier pipeline design
- **Phase Plant (Kilohearts):** 256-frame standard, wavetable editor with 15+ effects/modifiers, harmonic spectrum editing with phase control, pitch-detection import
- **CCRMA Stanford (Jatin Chowdhury):** Wavefolder mathematical analysis, sine vs triangle folder characteristics, antiderivative anti-aliasing for nonlinear processing
- **Dennis Klatt (1980):** Formant synthesis model, parallel/cascade resonator architecture, vowel formant frequency/amplitude/bandwidth tables
- **Csound Formant Table:** Quantitative F1-F5 frequencies, amplitudes, and bandwidths for soprano/alto/countertenor/tenor/bass across 5 vowels
- **Adam Szabo (2010):** "How to Emulate the Super Saw" -- Roland JP-8000 supersaw analysis, 7-oscillator detune algorithm
- **Hibberts/Lehr (1986):** Bell partial analysis, inharmonic partial group ratios for church bells and tubular bells
- **Peterson & Barney (1952):** Foundational vowel formant frequency measurements across speaker populations
- **KVR Audio Forum (t=517146):** Community-documented wavetable file format specifications, CLM chunk details, Surge WT format
- **Sound on Sound "Synthesizing Tonewheel Organs":** Hammond drawbar-to-harmonic mapping, classic registration recipes

---

## Risks and Alternatives

**Risk:** Memory consumption with 112 factory tables at full 256 frames each (potential 2+ GB).
**Fallback:** Use lazy loading -- only generate/load tables when selected. Store factory tables as compressed binary blobs. Limit most tables to 16-64 frames (adequate for many categories). Only evolving/textural tables need 128-256 frames.

**Risk:** Inharmonic bell/metallic wavetable mipmap artifacts due to non-integer partials.
**Fallback:** For inharmonic tables, generate mipmaps using a direct spectral lowpass approach (zero all FFT bins above the frequency threshold) rather than the harmonic-number-based approach. The existing code already does this correctly since it zeros bins by index, not by harmonic number.

**Risk:** Pitch detection fails on complex audio during import.
**Fallback:** Always offer manual frame-size entry as a fallback. Default to 2048 when auto-detection confidence is below threshold (autocorrelation < 0.8).

**Risk:** CLM chunk reading fails on non-standard WAV files.
**Fallback:** If CLM chunk is not found, fall back to fixed-length 2048-sample slicing. Offer a UI selector for manual frame-size override.

**Risk:** Modulation matrix per-voice processing exceeds CPU budget at 16 voices.
**Fallback:** Limit per-voice modulation to 8 routes maximum. Use block-rate (once per buffer) evaluation for slowly-varying sources (LFOs < 5Hz). Reserve sample-rate evaluation only for audio-rate modulation.

**Risk:** Wavetable editor complexity delays v2.0 significantly.
**Fallback:** Ship v1.5 with additive editor only (simplest to implement -- just harmonic sliders + IFFT). Add spectral and waveform drawing in v2.0. The additive editor alone provides significant value.

**Risk:** Spectral resynthesis produces poor results from noisy/complex audio.
**Fallback:** Implement a spectral envelope smoothing option (cepstral smoothing with adjustable lifter cutoff) that cleans up the spectral representation. Offer a "Simplify" parameter that progressively reduces the number of retained partials.

---

## Key File References

- Current wavetable data structure: `/Users/taylorbrook/Dev/VST-development/plugins/O-Prism/Source/dsp/WavetableData.h`
- Current wavetable generator: `/Users/taylorbrook/Dev/VST-development/plugins/O-Prism/Source/dsp/WavetableGenerator.h` and `.cpp`
- Current wavetable oscillator: `/Users/taylorbrook/Dev/VST-development/plugins/O-Prism/Source/dsp/WavetableOscillator.h` and `.cpp`
- Voice architecture: `/Users/taylorbrook/Dev/VST-development/plugins/O-Prism/Source/PrismVoice.h` and `.cpp`
- Processor (table assignment): `/Users/taylorbrook/Dev/VST-development/plugins/O-Prism/Source/PluginProcessor.cpp`
- Tuning engine: `/Users/taylorbrook/Dev/VST-development/plugins/O-Prism/Source/TuningEngine.h`
- Creative brief: `/Users/taylorbrook/Dev/VST-development/plugins/O-Prism/.planning/BRIEF.md`
- Development roadmap: `/Users/taylorbrook/Dev/VST-development/plugins/O-Prism/.planning/ROADMAP.md`