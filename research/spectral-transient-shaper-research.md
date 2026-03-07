---
title: "Spectral Transient Shaper Research"
created: 2026-02-03
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Deep research on frequency-selective transient processing combining FFT spectral analysis with per-band transient detection, covering oeksound Spiff, MolecularBytes AtomicTransient, and JUCE implementation strategies."
domain: dsp
type: research
keywords:
  - transient-shaper
  - spectral-processing
  - fft
  - per-band
  - dynamics
  - envelope-follower
  - transient-detection
  - juce
stages: [0, 2]
agents: [dsp, research]
---

# Spectral Transient Shaper - Deep Research Report

**Project:** Spectral Transient Shaper - Frequency-Selective Transient Processing Plugin
**Researched:** 2026-02-02
**Confidence Level:** HIGH (based on academic papers, commercial product analysis, DSP literature, and JUCE documentation)

---

## Executive Summary

Spectral transient shaping represents an emerging frontier in dynamics processing, combining FFT-based spectral analysis with per-band transient detection and manipulation. While traditional transient shapers operate on the full signal or a few broad frequency bands, spectral approaches allow surgical control over transients at specific frequencies. The market leaders in this space (oeksound Spiff, MolecularBytes AtomicTransient, Sonible entropy:EQ+) have demonstrated strong user demand for frequency-selective transient control. A well-designed spectral transient shaper could differentiate itself through unique visualization, intuitive control schemes, and optimized real-time performance.

---

## 1. Prior Art Survey - Existing Plugins

### 1.1 oeksound Spiff

**Category:** Adaptive Transient Processor
**Price:** $149 (often on sale for ~$79)
**Format:** VST, VST3, AU, AAX

**Core Technology:**
- Uses spectral analysis to identify transients in both time AND frequency domains
- Applies cuts or boosts using dynamic filters rather than traditional envelope-based processing
- Adapts to incoming signal in real-time; no fixed threshold

**Key Features:**
- Five EQ-style bands with frequency, bandwidth, and sensitivity controls
- Depth dial for main effect amount (not threshold-based)
- Sensitivity control: determines whether to affect only hard transients or softer hits
- Sharpness control: Q-value equivalent for transient detection precision
- Real-time graphical display showing processing activity

**Algorithm Philosophy:**
"Spiff analyzes the incoming signal and recognizes when and where transients appear adaptively. Its time- and frequency-specific controls let you take charge of your transients."

**Strengths:**
- Clean processing with minimal artifacts even at extreme settings
- Frequency-specific transient manipulation (unlike broadband shapers)
- Intuitive 5-band EQ-style interface
- Can both cut AND boost transients per band

**Weaknesses/User Complaints:**
- Can be CPU-intensive on complex material
- Learning curve for understanding "adaptive" behavior
- No per-band attack/release times
- Limited visual feedback on actual transient detection

**Sources:** [oeksound Official](https://oeksound.com/plugins/spiff/), [Sound On Sound Review](https://www.soundonsound.com/reviews/oeksound-spiff)

---

### 1.2 MolecularBytes AtomicTransient

**Category:** Polyphonic Multi-Channel Transient Processor
**Price:** ~$149
**Format:** VST, VST3, AU, AAX, Standalone

**Core Technology:**
- Spectral analysis engine that identifies individual transients even in complex mixes
- **Polyphonic spectral separation** - processes envelope of each "note" independently
- Up to three parallel processing channels, each with spectral filtering

**Key Features:**
- Per-channel spectral filtering for targeted transient detection
- Independent attack, sustain, decay processing per detected transient
- Time-based "waterfall diagram" showing historical transient representation
- Polyphonic mode: new notes don't affect already-sounding tones
- Real-time envelope visualization

**Algorithm Philosophy:**
"The polyphonic spectral separation, used for the first time in a transient shaper, allows the envelope of each polyphonic 'note' to be processed according to its own time base."

**Strengths:**
- True polyphonic processing (unprecedented in transient shapers)
- Excellent for piano, guitar, complex percussion
- Three independent processing channels
- Sophisticated visualization

**Weaknesses/User Complaints:**
- Complex interface with steep learning curve
- Higher CPU usage due to spectral separation
- May be overkill for simple drum processing
- Less intuitive than single-knob designs

**Sources:** [MolecularBytes Official](https://www.molecularbytes.com/mbcms/index.php/products/atomictransient), [KVR](https://www.kvraudio.com/product/atomictransient-by-molecular-bytes)

---

### 1.3 Sonible entropy:EQ+

**Category:** Frequency-Selective Transient/Harmonic EQ
**Price:** $129
**Format:** VST2, VST3, AU, AAX

**Core Technology:**
- 8-band linear phase EQ
- Differentiates between harmonic and inharmonic (transient/noise) sound portions
- Per-band control over transient content

**Key Features:**
- EQ interface with transient/sustain separation per band
- Can emphasize consonants in speech (inharmonic content)
- Isolate drum elements within complex mixes
- Amplify or attenuate string plucking/picking sounds

**Algorithm Philosophy:**
"entropy:EQ+ has the capacity to differentiate between harmonic and inharmonic sound portions, substantially simplifying things like the postproduction of transients."

**Strengths:**
- Familiar EQ-style workflow
- Linear phase (no phase distortion)
- Works on speech and music equally well
- Part of Sonible's "smart" ecosystem

**Weaknesses/User Complaints:**
- EQ paradigm may not be intuitive for transient work
- Less "transient shaper" feel, more "spectral EQ"
- 8 bands may not be enough for surgical work
- Higher latency due to linear phase

**Sources:** [Sonible Official](https://www.sonible.com/entropyeq/)

---

### 1.4 Eventide SplitEQ / Physion

**Category:** Transient/Tonal Separation EQ
**Price:** SplitEQ $179, Physion MkII $199
**Format:** VST, VST3, AU, AAX

**Core Technology:**
- **Structural Split** technology (US Patent No. 10,430,154 B2)
- Separates signal into Transient and Tonal paths
- 8-band parametric EQ applied independently to each path

**Key Features:**
- 14 "Source" presets optimized for different material (Kick, Vocals, Piano, etc.)
- Decay, Separation, and Smooth controls for detection tuning
- Each EQ band can affect Transient only, Tonal only, or both
- Physion adds effects processing per path

**Algorithm Philosophy:**
"Building on the Structural Split technology... separates the input signal into transient and tonal portions. This mimics the way we perceive sound."

**Strengths:**
- Patented separation algorithm (high quality)
- Intuitive "EQ the attack separately" workflow
- Excellent presets for different sources
- Physion allows creative effects per path

**Weaknesses/User Complaints:**
- Two paths only (not per-frequency transients)
- Relatively high CPU
- Expensive
- Separation not perfect on all material

**Sources:** [Eventide SplitEQ](https://www.eventideaudio.com/plug-ins/spliteq/)

---

### 1.5 Harrison Spectral Gate

**Category:** Intelligent Frequency-Dependent Gate
**Price:** $49
**Format:** VST2, VST3, AU, AAX

**Core Technology:**
- Learns spectral fingerprint of "Source" (desired) and "Spill" (unwanted)
- 24 adjustable nodes per curve across the spectrum
- Gates based on spectral matching, not just level

**Key Features:**
- Real-time spectrum display with Source (green) and Spill (red) curves
- Threshold, Depth, Hold, Release controls
- "Learn" buttons for Source and Spill
- Manually adjustable spectral nodes

**Algorithm Philosophy:**
"The Spectral Gate can learn and differentiate between the spectral characteristics of the desired 'Source' material and the undesired 'Spill' within a signal for easier and more accurate triggering."

**Strengths:**
- Affordable
- "Learning" approach is intuitive
- Excellent for drum isolation, vocal cleanup
- Sidechain compatible

**Weaknesses/User Complaints:**
- Gate, not transient shaper (different purpose)
- Fixed 24-node resolution
- Learning can fail on complex material
- No per-band timing controls

**Sources:** [Harrison Audio](https://harrisonaudio.com/plug-ins), [Sweetwater](https://www.sweetwater.com/store/detail/SpectralGate--harrison-audio-spectral-gate-plug-in)

---

### 1.6 Bayou Media Drum Gator

**Category:** Spectral Drum Gate/Transient Shaper
**Price:** $50 (Drum Gator II)
**Format:** VST, VST3, AU, AAX

**Core Technology:**
- Categorizes drum hits based on spectral content
- Transient detection based on rate-of-increase (not level threshold)
- "Spectral Signatures" learning mode

**Key Features:**
- Separate level control per spectral category
- Spectral Peak mode: manual frequency band sliders
- Spectral Signatures mode: learn individual drum characteristics
- Better transient detection than traditional gates

**Algorithm Philosophy:**
"Drum Gator categorizes individual drum hits based on their spectral (frequency) content and provides a separate level control for each category."

**Strengths:**
- Purpose-built for drums
- Transient-detection immune to varying hit intensities
- Can remove snare bleed from kick regardless of level
- Trainable spectral signatures

**Weaknesses/User Complaints:**
- Drum-focused (less versatile)
- UI dated
- Learning mode can be finicky
- Limited creative applications

**Sources:** [Bayou Media](https://bayou-media.com/drumgator), [Sound On Sound](https://www.soundonsound.com/reviews/bayou-media-drum-gator)

---

### 1.7 Auburn Sounds Couture

**Category:** Transient Shaper + Saturation
**Price:** $69 (Free version available)
**Format:** VST2, VST3, AU, AAX, CLAP, LV2

**Core Technology:**
- "Half-spectral, 2-band, RMS, program-dependent envelope follower"
- 100% program-dependent (no internal thresholds)
- Internal lookahead for precise transient alignment

**Key Features:**
- Front/Back control (combines attack and sustain concepts)
- Speed control (envelope timing)
- Three detector modes including "Human" (perceptual loudness)
- Dynamics-preserving saturation section

**Algorithm Philosophy:**
"A 2-bands, half-spectral, RMS, program-dependent detector that finds each and every transient. No internal thresholds."

**Strengths:**
- Free version available
- Novel Front/Back paradigm
- "Human" detector mode
- Saturation integration

**Weaknesses/User Complaints:**
- Not truly spectral (2-band only)
- Front/Back less intuitive than Attack/Sustain
- Limited frequency selectivity
- No visual transient display

**Sources:** [Auburn Sounds](https://www.auburnsounds.com/products/Couture.html)

---

### 1.8 Common User Complaints Across Products

From KVR and Gearspace forums:

1. **"Multiband shapers sound unnatural"** - Fixed crossover frequencies don't match musical content
2. **"Can't see what's happening"** - Lack of visual feedback on transient detection
3. **"Too much latency"** - FFT-based processors introduce delay
4. **"CPU hog"** - Spectral processing is computationally expensive
5. **"Can't get super fast timings"** - FFT window size limits transient resolution
6. **"Gets phasey"** - Phase coherence issues in multiband designs
7. **"No resize"** - UI scaling complaints
8. **"Learning curve is steep"** - Complex interfaces

---

## 2. Algorithm Research

### 2.1 Transient Detection Methods

#### 2.1.1 Envelope Follower (Time Domain)

**Basic Principle:**
Two envelope followers with different attack times; subtracting slow from fast reveals transients.

**SPL Differential Envelope Method:**
```
1. Bandpass input into frequency bands
2. Compute derivative of power in each band
3. Create fast envelope (attack ~1ms) and slow envelope (attack ~15ms)
4. Subtract slow from fast = transient signal
5. Use difference to modulate original signal
```

**Pros:** Low latency, simple, proven
**Cons:** No true frequency selectivity within bands, phase issues at crossovers

**Sources:** [GitHub multiband-transient-shaper](https://github.com/sevagh/multiband-transient-shaper)

---

#### 2.1.2 Spectral Flux (Frequency Domain)

**Basic Principle:**
Measure how much the spectrum changes between consecutive FFT frames.

**Algorithm:**
```
spectral_flux[n] = sum( max(0, |X[n,k]| - |X[n-1,k]|) ) for all bins k
```
Only count positive changes (energy increases indicate onsets).

**Pros:** Catches transients at specific frequencies, robust
**Cons:** Requires FFT (latency), time resolution limited by hop size

**Sources:** [Wikipedia - Spectral Flux](https://en.wikipedia.org/wiki/Spectral_flux), DAFx papers

---

#### 2.1.3 Complex Domain Detection

**Basic Principle:**
Combine magnitude AND phase information; phase deviation indicates new events.

**Algorithm (from Bello et al. 2003):**
```
1. Predict expected complex value based on previous frames
2. Measure deviation between predicted and actual
3. Large deviations = onset/transient

Detection function = |X[n,k] - X_predicted[n,k]|
```

**Pros:** More accurate than magnitude-only, catches pitched transients
**Cons:** Complex implementation, phase unwrapping issues

**Sources:** [Complex Domain Onset Detection - DAFx 2003](https://www.researchgate.net/publication/200806123_Complex_Domain_Onset_Detection_for_Musical_Signals)

---

#### 2.1.4 Weighted Phase Deviation

**Basic Principle:**
Weight phase changes by magnitude (ignore phase noise in quiet bins).

**Algorithm:**
```
WPD = sum( |X[n,k]| * |phase_deviation[n,k]| ) / sum(|X[n,k]|)
```

**Pros:** More robust to noise than raw phase deviation
**Cons:** Still requires FFT, computational overhead

**Sources:** [Onset Detection Revisited - DAFx 2006](https://www.dafx.de/paper-archive/2006/papers/p_133.pdf)

---

### 2.2 Per-Band Transient Detection in FFT Domain

#### Approach 1: Per-Bin Envelope Followers

For each FFT bin, maintain a separate envelope follower:
```cpp
for (int bin = 0; bin < numBins; ++bin) {
    float magnitude = std::abs(fftData[bin]);
    float fastEnv = fastEnvelope[bin].process(magnitude);
    float slowEnv = slowEnvelope[bin].process(magnitude);
    transientGain[bin] = computeGain(fastEnv - slowEnv);
}
```

**Challenges:**
- FFT bins have poor time resolution for fast transients
- Small FFT = better time resolution but worse frequency resolution
- Need smoothing across bins to avoid "stripey" artifacts

---

#### Approach 2: Spectral Flux Per Band

Group FFT bins into bands (linear, logarithmic, Bark scale, etc.):
```cpp
for (int band = 0; band < numBands; ++band) {
    float flux = 0;
    for (int bin = bandStart[band]; bin < bandEnd[band]; ++bin) {
        float diff = magnitude[bin] - prevMagnitude[bin];
        if (diff > 0) flux += diff;
    }
    transientActivity[band] = flux / (bandEnd[band] - bandStart[band]);
}
```

**Advantages:** Reduces computation, provides meaningful frequency groupings
**Trade-off:** Less precision than per-bin, but more stable

---

#### Approach 3: Multi-Resolution Analysis

Use different FFT sizes for different frequency ranges:
- **Low frequencies:** Large FFT (4096) for good frequency resolution
- **High frequencies:** Small FFT (256) for good time resolution

**Implementation:** Parallel STFT with different window sizes, combine results

**Sources:** [Multiresolution Analysis - Wikipedia](https://en.wikipedia.org/wiki/Multiresolution_analysis)

---

### 2.3 Attack/Sustain Separation

#### Eventide's Structural Split Approach

1. Compute magnitude spectrogram
2. Apply median filtering in time direction (captures tonal/sustain)
3. Subtract median from original = transient residual
4. Reconstruct audio from each component separately

**Key insight:** Tonal content is consistent across time; transients are brief spikes.

---

#### IRCAM Sines + Noise + Transients Model

1. **Sines:** Track spectral peaks across frames (partial tracking)
2. **Transients:** Detect via spectral flux or phase coherence
3. **Noise:** Residual after removing sines and transients

Each component can be processed independently and recombined.

**Sources:** [IRCAM - Peak Classification](http://recherche.ircam.fr/anasyn/roebel/paper/peakclass_icmc2004.pdf)

---

### 2.4 Applying Transient Shaping Per FFT Bin

#### Method 1: Magnitude Scaling

```cpp
// For each bin, compute transient gain
float transientAmount = detectTransient(bin);
float gain = 1.0f + (attackBoost * transientAmount);

// Apply to magnitude only, preserve phase
fftData[bin] *= gain;
```

**Pros:** Simple, preserves phase relationships
**Cons:** Can sound "filtered" if not smoothed

---

#### Method 2: Dynamic Filtering (Spiff-style)

Instead of scaling magnitudes, apply dynamic EQ curves that change based on transient detection:

```cpp
// When transient detected in frequency range:
// Temporarily boost/cut that range with fast attack, slow release
filterGain[band] = envelope.process(
    1.0f + transientBoost * transientDetected[band]
);
```

**Pros:** Smoother, more musical
**Cons:** More complex, potential phase issues

---

#### Method 3: Separate Path Processing

1. Extract transient component (via spectral methods)
2. Extract sustain component
3. Process each independently (different compression, EQ, etc.)
4. Recombine with adjustable mix

**Pros:** Maximum flexibility
**Cons:** Highest computational cost, artifacts at boundaries

---

## 3. Academic/Technical Sources

### 3.1 Key Papers

1. **"Complex Domain Onset Detection for Musical Signals"**
   - Authors: Duxbury, Bello, Davies, Sandler (2003)
   - Conference: DAFx-03
   - Key contribution: Combined magnitude and phase for robust onset detection
   - [ResearchGate](https://www.researchgate.net/publication/200806123_Complex_Domain_Onset_Detection_for_Musical_Signals)

2. **"On the Use of Phase and Energy for Musical Onset Detection in the Complex Domain"**
   - Authors: Bello, Duxbury, Davies, Sandler (2004)
   - Journal: IEEE Signal Processing Letters
   - Key contribution: Improved onset detection combining energy and phase
   - [Semantic Scholar](https://www.semanticscholar.org/paper/On-the-use-of-phase-and-energy-for-musical-onset-in-Bello-Duxbury/488033ec427e85377cb95a7e5a5d8c5b77b7056a)

3. **"Onset Detection Revisited"**
   - Authors: Dixon (2006)
   - Conference: DAFx-06
   - Key contribution: Weighted phase deviation, half-wave rectified complex difference
   - [DAFx Paper](https://www.dafx.de/paper-archive/2006/papers/p_133.pdf)

4. **"A New Approach to Transient Processing in the Phase Vocoder"**
   - Author: Roebel (2003)
   - Key contribution: Phase reinitialization for transient preservation
   - [MP3-Tech](https://www.mp3-tech.org/programmer/docs/dafx32.pdf)

5. **"Transient Detection and Preservation in the Phase Vocoder"**
   - Authors: Laroche, Dolson (2003)
   - Conference: ICMC 2003
   - Key contribution: Phase locking for vertical coherence
   - [HAL Science](https://hal.science/hal-01161125/document)

6. **"Phase Vocoder Done Right"**
   - Authors: Pruska, Holighaus (2022)
   - Key contribution: Modern improvements to phase vocoder
   - [arXiv](https://arxiv.org/pdf/2202.07382)

7. **"Drum Source Separation Using Percussive Feature Detection"**
   - Key contribution: Spectral modulation for drum isolation
   - [ResearchGate](https://www.researchgate.net/publication/228545344_Drum_source_separation_using_percussive_feature_detection_and_spectral_modulation)

---

### 3.2 Books and Online Resources

1. **"Spectral Audio Signal Processing"** by Julius O. Smith III
   - Free online: [CCRMA Stanford](https://ccrma.stanford.edu/~jos/sasp/)
   - Covers STFT, overlap-add, phase vocoder, spectral modeling

2. **"DAFX: Digital Audio Effects"** (Edited by Zolzer)
   - Comprehensive DSP reference for audio effects

3. **"The Audio Programming Book"** by Boulanger & Lazzarini
   - Practical implementations in C/Csound

4. **DSPRelated.com**
   - [FFT Filter Banks](https://www.dsprelated.com/freebooks/sasp/FFT_Filter_Banks.html)
   - [Overlap-Add Processing](https://www.dsprelated.com/freebooks/sasp/Overlap_Add_OLA_STFT_Processing.html)

---

## 4. JUCE Implementation

### 4.1 juce::dsp::FFT Usage

```cpp
// Setup
static constexpr int fftOrder = 10;  // 2^10 = 1024 points
static constexpr int fftSize = 1 << fftOrder;  // 1024
juce::dsp::FFT forwardFFT { fftOrder };
juce::dsp::FFT inverseFFT { fftOrder };

// Process
std::array<float, fftSize * 2> fftData;  // Real + imaginary interleaved

// Forward transform
forwardFFT.performRealOnlyForwardTransform(fftData.data());

// Access bins (after forward transform)
for (int i = 0; i < fftSize / 2; ++i) {
    float real = fftData[i * 2];
    float imag = fftData[i * 2 + 1];
    float magnitude = std::sqrt(real * real + imag * imag);
    float phase = std::atan2(imag, real);

    // Process magnitude/phase here...

    // Reconstruct
    fftData[i * 2] = magnitude * std::cos(phase);
    fftData[i * 2 + 1] = magnitude * std::sin(phase);
}

// Inverse transform
inverseFFT.performRealOnlyInverseTransform(fftData.data());
```

**Key Points:**
- FFT object has overhead; create once and reuse
- performRealOnlyForwardTransform is optimized for real signals
- JUCE IFFT divides by fftSize; account for this in gain staging

**Sources:** [JUCE FFT Tutorial](https://docs.juce.com/master/tutorial_simple_fft.html)

---

### 4.2 Overlap-Add STFT Implementation

```cpp
class STFTProcessor {
    static constexpr int fftSize = 1024;
    static constexpr int hopSize = 256;  // 75% overlap
    static constexpr int numOverlaps = fftSize / hopSize;  // 4

    juce::dsp::FFT fft { 10 };
    juce::dsp::WindowingFunction<float> window {
        fftSize, juce::dsp::WindowingFunction<float>::hann
    };

    std::array<float, fftSize> inputFifo;
    std::array<float, fftSize> outputFifo;
    std::array<float, fftSize * 2> fftData;
    int fifoIndex = 0;

public:
    float processSample(float input) {
        // Write to input FIFO
        inputFifo[fifoIndex] = input;

        // Read from output FIFO (delayed)
        float output = outputFifo[fifoIndex];
        outputFifo[fifoIndex] = 0.0f;  // Clear for next overlap

        if (++fifoIndex >= hopSize) {
            fifoIndex = 0;
            processFrame();
        }

        return output;
    }

private:
    void processFrame() {
        // Copy and window input
        std::copy(inputFifo.begin(), inputFifo.end(), fftData.begin());
        window.multiplyWithWindowingTable(fftData.data(), fftSize);

        // Forward FFT
        fft.performRealOnlyForwardTransform(fftData.data());

        // *** SPECTRAL PROCESSING HERE ***
        processSpectrum(fftData);

        // Inverse FFT
        fft.performRealOnlyInverseTransform(fftData.data());

        // Window again (synthesis window)
        window.multiplyWithWindowingTable(fftData.data(), fftSize);

        // Overlap-add to output
        for (int i = 0; i < fftSize; ++i) {
            int idx = (fifoIndex + i) % fftSize;
            outputFifo[idx] += fftData[i] * (1.0f / numOverlaps);
        }

        // Shift input FIFO
        std::rotate(inputFifo.begin(), inputFifo.begin() + hopSize, inputFifo.end());
    }
};
```

**Latency:** fftSize samples (1024 @ 44.1kHz = ~23ms)

**Sources:** [audiodev.blog FFT Processing](https://audiodev.blog/fft-processing/), [GitHub hollance/fft-juce](https://github.com/hollance/fft-juce)

---

### 4.3 Windowing Functions

| Window | Dynamic Range | Resolution | Use Case |
|--------|--------------|------------|----------|
| Rectangular | Low | High | Transient analysis (no weighting) |
| Hann | Good | Fair | General purpose, smooth |
| Hamming | Fair | Good | Narrow-band applications |
| Blackman | High | Low | Wide-band, low leakage |

**For transient shaping:** Hann or Hamming with 75% overlap recommended.

**JUCE Windowing:**
```cpp
juce::dsp::WindowingFunction<float> window {
    fftSize,
    juce::dsp::WindowingFunction<float>::hann,
    false  // not normalized
};
```

---

### 4.4 CPU Optimization Strategies

#### 4.4.1 SIMD with SIMDRegister

```cpp
#include <juce_dsp/juce_dsp.h>

// Process 4 floats at once (SSE) or 8 (AVX)
using SIMDFloat = juce::dsp::SIMDRegister<float>;

void processBlock(float* data, int numSamples) {
    constexpr auto simdSize = SIMDFloat::size();  // 4 for SSE, 8 for AVX

    for (int i = 0; i < numSamples; i += simdSize) {
        auto simdData = SIMDFloat::fromRawArray(data + i);

        // SIMD operations
        simdData = simdData * gain;

        simdData.copyToRawArray(data + i);
    }
}
```

**Important:** Ensure 16-byte alignment for SSE, 32-byte for AVX.

**Sources:** [JUCE SIMD Tutorial](https://docs.juce.com/master/tutorial_simd_register_optimisation.html)

---

#### 4.4.2 FloatVectorOperations

For simpler cases, use JUCE's pre-optimized functions:

```cpp
juce::FloatVectorOperations::multiply(dest, src, gain, numSamples);
juce::FloatVectorOperations::add(dest, src, numSamples);
juce::FloatVectorOperations::copy(dest, src, numSamples);
```

---

#### 4.4.3 Reducing FFT Computation

1. **Process every N hops:** Skip FFT processing on some frames, interpolate
2. **Reduce FFT size for high frequencies:** Multi-resolution approach
3. **Use smaller FFT with more overlap:** Trade-off between frequency resolution and CPU
4. **Partition processing:** Use non-uniform partitioned convolution techniques

---

#### 4.4.4 Threading Considerations

- FFT processing can be moved to a background thread
- Use lock-free FIFOs to communicate between audio and processing threads
- Consider juce::dsp::ProcessorDuplicator for parallel voice processing

---

### 4.5 Per-Band Transient Detection Implementation

```cpp
class SpectralTransientDetector {
    static constexpr int numBands = 32;
    static constexpr int fftSize = 1024;

    struct Band {
        float prevMagnitude = 0.0f;
        float fastEnvelope = 0.0f;
        float slowEnvelope = 0.0f;
        float transientGain = 1.0f;
    };

    std::array<Band, numBands> bands;
    std::array<int, numBands + 1> bandBoundaries;

    // Parameters
    float sensitivity = 0.5f;
    float attackBoost = 1.0f;  // dB
    float sustainBoost = 0.0f; // dB
    float fastAttack = 0.001f; // seconds
    float slowAttack = 0.015f; // seconds
    float release = 0.020f;    // seconds

public:
    void processSpectrum(float* fftData, int numBins, float sampleRate) {
        float hopTime = float(fftSize / 4) / sampleRate;  // Assuming 75% overlap

        for (int band = 0; band < numBands; ++band) {
            // Calculate band magnitude
            float bandMag = 0.0f;
            for (int bin = bandBoundaries[band]; bin < bandBoundaries[band + 1]; ++bin) {
                float real = fftData[bin * 2];
                float imag = fftData[bin * 2 + 1];
                bandMag += std::sqrt(real * real + imag * imag);
            }
            bandMag /= (bandBoundaries[band + 1] - bandBoundaries[band]);

            // Spectral flux (positive only)
            float flux = std::max(0.0f, bandMag - bands[band].prevMagnitude);
            bands[band].prevMagnitude = bandMag;

            // Envelope followers
            float fastCoeff = std::exp(-hopTime / fastAttack);
            float slowCoeff = std::exp(-hopTime / slowAttack);
            float releaseCoeff = std::exp(-hopTime / release);

            if (flux > bands[band].fastEnvelope)
                bands[band].fastEnvelope = flux;
            else
                bands[band].fastEnvelope *= releaseCoeff;

            if (flux > bands[band].slowEnvelope)
                bands[band].slowEnvelope += (1.0f - slowCoeff) * (flux - bands[band].slowEnvelope);
            else
                bands[band].slowEnvelope *= releaseCoeff;

            // Transient = fast - slow
            float transient = bands[band].fastEnvelope - bands[band].slowEnvelope;
            transient = std::max(0.0f, transient) * sensitivity;

            // Calculate gain
            float attackGain = juce::Decibels::decibelsToGain(attackBoost * transient);
            float sustainGain = juce::Decibels::decibelsToGain(sustainBoost * (1.0f - transient));
            bands[band].transientGain = attackGain * sustainGain;

            // Apply gain to FFT bins
            for (int bin = bandBoundaries[band]; bin < bandBoundaries[band + 1]; ++bin) {
                fftData[bin * 2] *= bands[band].transientGain;
                fftData[bin * 2 + 1] *= bands[band].transientGain;
            }
        }
    }

    void setupBandBoundaries(int numBins, float sampleRate) {
        // Logarithmic band distribution (approximating Bark scale)
        float minFreq = 20.0f;
        float maxFreq = sampleRate / 2.0f;
        float logMin = std::log10(minFreq);
        float logMax = std::log10(maxFreq);

        for (int i = 0; i <= numBands; ++i) {
            float logFreq = logMin + (logMax - logMin) * i / numBands;
            float freq = std::pow(10.0f, logFreq);
            int bin = int(freq * numBins / (sampleRate / 2.0f));
            bandBoundaries[i] = std::clamp(bin, 0, numBins);
        }
    }
};
```

---

## 5. Differentiation Opportunities

### 5.1 Visual Feedback Ideas

#### 5.1.1 Spectrogram with Transient Highlighting

Display a scrolling spectrogram where:
- Brightness = magnitude
- **Color overlay** = transient detection intensity (e.g., red = high transient activity)
- User can see exactly WHERE and WHEN transients are detected

**Implementation:** Use juce::Image with OpenGL for efficient rendering.

---

#### 5.1.2 3D Transient Waterfall

Similar to AtomicTransient's approach:
- X-axis: frequency
- Y-axis: time (scrolling)
- Z-axis (height/color): transient intensity

---

#### 5.1.3 Per-Band Envelope Display

Show envelope followers for each band:
- Fast envelope (thin line)
- Slow envelope (thick line)
- Difference = transient (filled area)

---

#### 5.1.4 Before/After Waveform Overlay

Real-time display showing:
- Input waveform (ghost)
- Output waveform (solid)
- Clearly shows attack boost/cut and sustain changes

---

### 5.2 Novel Control Schemes

#### 5.2.1 Draw Attack/Sustain Curves

**Similar to EQuivocate's "Draw Curve" mode:**
- X-axis: frequency (log scale)
- Y-axis: attack boost/cut amount
- Second layer for sustain boost/cut
- Draw freehand or use nodes

**Advantage:** Intuitive, no band-count limitations, familiar EQ paradigm

---

#### 5.2.2 "Spectral Transient Masks"

Pre-defined transient profiles for common sounds:
- "Kick Attack": Boost 50-100Hz transients
- "Snare Crack": Boost 2-5kHz transients
- "Hi-Hat Bite": Boost 8-16kHz transients
- Users can save/load custom masks

---

#### 5.2.3 Sidechain-Driven Spectral Shaping

Use sidechain input to define "target" transient profile:
- Plugin analyzes sidechain transients
- Applies similar transient shape to main input
- "Make this sound punch like that sound"

---

#### 5.2.4 AI-Assisted Detection

Machine learning model that:
- Identifies instrument types
- Auto-selects appropriate transient detection settings
- Suggests attack/sustain adjustments

---

### 5.3 Integration with Other Spectral Effects

#### Spectral Transient Shaper + Spectral Gate

Combined workflow:
1. Gate removes unwanted bleed
2. Transient shaper enhances remaining content
3. Single plugin for drum cleanup

---

#### Spectral Transient Shaper + Spectral Compression

Per-band dynamics:
- Transient detection triggers compression
- Compress only sustained content, preserve transients
- Or vice versa: limit transients, expand sustain

---

#### Spectral Transient Shaper + Harmonic Exciter

Apply exciter only to:
- Transient portions (add "crack" to snares)
- Or sustain portions (add "warmth" to tails)

---

## 6. Parameter Design

### 6.1 Essential Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Attack** | -24 to +24 dB | 0 dB | Boost/cut transient portion |
| **Sustain** | -24 to +24 dB | 0 dB | Boost/cut sustained portion |
| **Sensitivity** | 0-100% | 50% | How easily transients are detected |
| **Speed** | 0.1-50 ms | 5 ms | Envelope follower timing |
| **Mix** | 0-100% | 100% | Wet/dry blend |

---

### 6.2 Per-Band Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Band Attack** | -24 to +24 dB | 0 dB | Attack boost/cut for this band |
| **Band Sustain** | -24 to +24 dB | 0 dB | Sustain boost/cut for this band |
| **Band Sensitivity** | 0-200% | 100% | Relative to global sensitivity |
| **Band Solo** | On/Off | Off | Solo this band |
| **Band Bypass** | On/Off | Off | Bypass processing for this band |

---

### 6.3 Global Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **FFT Size** | 512/1024/2048/4096 | 1024 | Trade-off: time vs. frequency resolution |
| **Overlap** | 2x/4x/8x | 4x | Higher = smoother but more CPU |
| **Band Count** | 8/16/32/64 | 32 | Number of frequency bands |
| **Band Scale** | Linear/Log/Bark | Log | Band distribution |
| **Lookahead** | 0-20 ms | 5 ms | Delay for transient anticipation |
| **Output Gain** | -12 to +12 dB | 0 dB | Compensate for level changes |

---

### 6.4 Detection Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Detection Mode** | Envelope/Flux/Complex | Flux | Algorithm for transient detection |
| **Fast Attack** | 0.1-5 ms | 1 ms | Fast envelope attack time |
| **Slow Attack** | 5-50 ms | 15 ms | Slow envelope attack time |
| **Release** | 5-200 ms | 20 ms | Envelope release time |
| **Threshold** | -60 to 0 dB | -40 dB | Ignore transients below this level |

---

### 6.5 Visual Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Spectrogram Display** | On/Off | On | Show scrolling spectrogram |
| **Transient Overlay** | On/Off | On | Show transient detection overlay |
| **Envelope Display** | On/Off | Off | Show per-band envelopes |
| **Metering** | Peak/RMS/Both | Both | Input/output metering |

---

## 7. Implementation Recommendations

### 7.1 Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Audio Thread                             │
├─────────────────────────────────────────────────────────────┤
│  Input → Input FIFO → [Background Thread Signal]            │
│                                                              │
│  Output FIFO → Output (latency-compensated)                 │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                   Processing Thread                          │
├─────────────────────────────────────────────────────────────┤
│  1. Read from Input FIFO                                    │
│  2. Apply analysis window                                    │
│  3. Forward FFT                                              │
│  4. Per-band transient detection                            │
│  5. Apply gains                                              │
│  6. Inverse FFT                                              │
│  7. Apply synthesis window                                   │
│  8. Overlap-add to Output FIFO                              │
│  9. Send visualization data to GUI                          │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                      GUI Thread                              │
├─────────────────────────────────────────────────────────────┤
│  1. Receive visualization data (lock-free)                  │
│  2. Update spectrogram                                       │
│  3. Update meters                                            │
│  4. Handle user input                                        │
│  5. Send parameter changes (lock-free)                      │
└─────────────────────────────────────────────────────────────┘
```

---

### 7.2 Development Phases

#### Phase 1: Core STFT Engine
- Implement basic overlap-add STFT
- Verify perfect reconstruction (bypass mode)
- Measure latency and CPU usage

#### Phase 2: Basic Transient Detection
- Implement spectral flux detection
- Single-band transient shaping (proof of concept)
- Basic attack/sustain controls

#### Phase 3: Multi-Band Processing
- Add band splitting (logarithmic)
- Per-band envelope followers
- Per-band gain application

#### Phase 4: Visualization
- Spectrogram display
- Transient overlay
- Real-time metering

#### Phase 5: Advanced Features
- Multiple detection algorithms
- Drawable curves
- Presets and masks

#### Phase 6: Optimization
- SIMD optimization
- Threading
- Profile and optimize hot paths

---

### 7.3 Testing Strategy

1. **Unit Tests:**
   - FFT/IFFT reconstruction accuracy
   - Envelope follower behavior
   - Parameter smoothing

2. **Audio Tests:**
   - White noise (verify no coloration in bypass)
   - Impulse response (verify transient preservation)
   - Drum loops (verify attack/sustain control)
   - Piano (verify polyphonic behavior)

3. **Performance Tests:**
   - CPU usage at various FFT sizes
   - Latency measurement
   - Real-time safety (no dropouts)

4. **A/B Comparison:**
   - Compare against Spiff, AtomicTransient
   - Blind listening tests

---

## 8. Conclusion

A spectral transient shaper represents a compelling plugin concept with clear market demand and room for innovation. The key challenges are:

1. **Latency:** FFT processing inherently adds delay; must be managed carefully
2. **CPU:** Per-bin processing is expensive; optimization is critical
3. **Artifacts:** Phase coherence and transient smearing must be addressed
4. **UX:** Complex spectral processing needs intuitive controls

**Differentiation opportunities:**
- Superior visualization (spectrogram + transient overlay)
- Drawable attack/sustain curves (novel control scheme)
- AI-assisted detection presets
- Integration with other spectral effects

**Recommended starting point:**
- 1024-sample FFT with 75% overlap (~23ms latency at 44.1kHz)
- 32 logarithmic bands
- Spectral flux detection
- Global attack/sustain with per-band overrides
- Spectrogram visualization with transient highlighting

This approach balances complexity, performance, and user experience while leaving room for future expansion.

---

## 9. Sources

### Commercial Products
- [oeksound Spiff](https://oeksound.com/plugins/spiff/)
- [MolecularBytes AtomicTransient](https://www.molecularbytes.com/mbcms/index.php/products/atomictransient)
- [Sonible entropy:EQ+](https://www.sonible.com/entropyeq/)
- [Eventide SplitEQ](https://www.eventideaudio.com/plug-ins/spliteq/)
- [Harrison Spectral Gate](https://harrisonaudio.com/plug-ins)
- [Auburn Sounds Couture](https://www.auburnsounds.com/products/Couture.html)
- [Bayou Media Drum Gator](https://bayou-media.com/drumgator)

### Technical Resources
- [JUCE FFT Tutorial](https://docs.juce.com/master/tutorial_simple_fft.html)
- [JUCE SIMD Tutorial](https://docs.juce.com/master/tutorial_simd_register_optimisation.html)
- [audiodev.blog FFT Processing](https://audiodev.blog/fft-processing/)
- [CCRMA Spectral Audio Signal Processing](https://ccrma.stanford.edu/~jos/sasp/)
- [DSPRelated Overlap-Add](https://www.dsprelated.com/freebooks/sasp/Overlap_Add_OLA_STFT_Processing.html)

### Academic Papers
- [Complex Domain Onset Detection - DAFx 2003](https://www.researchgate.net/publication/200806123_Complex_Domain_Onset_Detection_for_Musical_Signals)
- [Onset Detection Revisited - DAFx 2006](https://www.dafx.de/paper-archive/2006/papers/p_133.pdf)
- [Phase Vocoder Transient Preservation](https://hal.science/hal-01161125/document)
- [Drum Source Separation](https://www.researchgate.net/publication/228545344_Drum_source_separation_using_percussive_feature_detection_and_spectral_modulation)

### Forums and Discussions
- [KVR Transient Shaper Discussion](https://www.kvraudio.com/forum/viewtopic.php?t=605735)
- [KVR Multiband Transient Shaper](https://www.kvraudio.com/forum/viewtopic.php?t=212697)
- [KVR Spectral Dynamics Processing](https://www.kvraudio.com/forum/viewtopic.php?t=531986)
