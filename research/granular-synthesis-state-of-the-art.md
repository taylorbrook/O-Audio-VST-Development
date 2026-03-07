---
title: "Granular Synthesis: State of the Art"
created: 2026-02-08
domain: dsp
type: research
keywords:
  - granular-synthesis
  - grain-scheduling
  - windowing
  - pitch-shifting
  - time-stretching
---
# Granular Synthesis: State of the Art

**Researched:** 2026-02-07
**Domain:** Granular synthesis theory, algorithms, implementation, commercial landscape
**Target:** JUCE C++ audio plugin development
**Confidence:** HIGH (academic sources) / MEDIUM (commercial analysis, optimization benchmarks)

---

## Table of Contents

1. [Academic Foundations](#1-academic-foundations)
2. [Modern Granular Techniques (2023-2026)](#2-modern-granular-techniques-2023-2026)
3. [Real-Time Implementation Strategies](#3-real-time-implementation-strategies)
4. [Anti-Aliasing for Pitched Grains](#4-anti-aliasing-for-pitched-grains)
5. [Commercial Granular Plugins](#5-commercial-granular-plugins)
6. [Open-Source Implementations](#6-open-source-implementations)
7. [Advanced Grain Envelope Shapes](#7-advanced-grain-envelope-shapes)
8. [Time-Stretching and Pitch-Shifting Algorithms](#8-time-stretching-and-pitch-shifting-algorithms)
9. [Spatial and Multichannel Granular](#9-spatial-and-multichannel-granular)
10. [Performance Optimization](#10-performance-optimization)

---

## 1. Academic Foundations

### 1.1 Dennis Gabor - The Theoretical Origin (1947)

**Key Paper:** "Acoustical Quanta and the Theory of Hearing" (1947)

Dennis Gabor proposed that sound signals can be represented as assemblies of short, localized wave packets or "quanta." His original intent was reducing data requirements for telecommunications, but the theory became the mathematical foundation for granular synthesis. Gabor demonstrated that any sound can be decomposed into elementary "acoustical quanta" -- short wavelets with both time and frequency localization. This is the earliest formalization of the uncertainty principle applied to acoustics: a sound quantum cannot be simultaneously precise in both time and frequency.

**Key Insight for Implementation:** Gabor's quantum concept maps directly to the grain -- a short windowed waveform with finite duration and controllable frequency content. The grain duration determines the trade-off between temporal precision and spectral smearing.

**Source:** Gabor, D. (1947). "Acoustical Quanta and the Theory of Hearing." *Nature*, 159(4044), 591-594.

### 1.2 Iannis Xenakis - Compositional Theory (1959-1971)

**Key Works:** *Formalized Music* (1971), *Analogique A-B* for string orchestra and tape (1959)

Xenakis was the first to formulate a compositional theory for granular sound, beginning with the lemma: "All sound, even continuous musical variation, is conceived as an assemblage of a large number of elementary sounds adequately disposed in time." He created granular sounds using analog tone generators and tape splicing, applying stochastic mathematics (probability distributions, Markov chains) to control grain parameters.

**Key Insight for Implementation:** Xenakis's stochastic approach maps directly to parameter randomization in modern granular engines. His use of probability distributions for grain density, pitch, and duration remains the standard approach for asynchronous granular synthesis. Poisson processes for inter-onset times create natural-sounding irregularity.

**Source:** [Iannis Xenakis - Granular Synthesis](https://www.iannis-xenakis.org/en/granular-synthesis/)

### 1.3 Curtis Roads - Digital Implementation and Microsound (1974-2001)

**Key Works:**
- First computer implementation of granular synthesis (1974)
- *The Computer Music Tutorial* (MIT Press, 1996)
- *Microsound* (MIT Press, 2001)

Roads was the first to implement granular synthesis on a computer. His book *Microsound* is the definitive academic text, covering:

| Chapter | Topic | Implementation Relevance |
|---------|-------|--------------------------|
| Ch. 1-3 | Time scales, history of microsound | Theoretical grounding for grain duration choices |
| Ch. 4-5 | Particle types (glissons, grainlets, pulsars, trainlets), synthesis/transformation | Distinct grain generation algorithms beyond basic windowed samples |
| Ch. 6 | STFT, phase vocoding, Gabor transform | Spectral granular hybrid approaches |
| Ch. 7-9 | Composition, aesthetics, future directions | Design philosophy |

Roads's software projects include **Cloudgenerator**, **Pulsargenerator**, and **EmissionControl2** (the latter still actively maintained and open source).

**Key Insight for Implementation:** Roads distinguishes between multiple microsound particle types. A "grainlet" is a grain with an internal waveform (sine, FM, etc.) as opposed to a sample-based grain. "Pulsars" use a duty-cycle approach where a waveform occupies only part of the grain period. These distinctions suggest implementation architectures where grains have pluggable source generators.

**Sources:**
- Roads, C. (2001). *Microsound*. MIT Press. ISBN: 978-0262681544
- [Curtis Roads - Microsound (MIT Press)](https://mitpress.mit.edu/9780262681544/microsound/)
- [Sound on Sound Review](https://www.soundonsound.com/reviews/curtis-roads-microsound)

### 1.4 Barry Truax - Real-Time Granular Synthesis (1986-1988)

**Key Paper:** "Real-time granular synthesis with a digital signal processing computer" (1988)

Truax implemented the first real-time granular synthesis system using the DMX-1000 Signal Processing Computer in 1986. His work established the practical framework for real-time grain scheduling and introduced the concept of grain density as a primary compositional parameter.

**Key Insight for Implementation:** Truax demonstrated that grain density (grains per second) is the primary perceptual parameter:
- **< 20 grains/sec:** Individual grains are perceptible as rhythmic pulses
- **20-100 grains/sec:** Transitional zone, texture begins to fuse
- **> 100 grains/sec:** Continuous sound mass, individual grains imperceptible

This density threshold maps directly to UI design -- users need fundamentally different control paradigms below and above ~20 Hz density.

**Sources:**
- Truax, B. (1988). "Real-time granular synthesis with a digital signal processing computer." *Computer Music Journal*, 12(2), 14-26.
- [Barry Truax - Granular Synthesis](https://www.sfu.ca/~truax/gran.html)

### 1.5 Roads - Evolution of Granular Synthesis (Overview Paper)

**Source:** [The Evolution of Granular Synthesis: An Overview of Current Research (Semantic Scholar)](https://www.semanticscholar.org/paper/The-evolution-of-granular-synthesis:-an-overview-of-Roads/36ea94a82ce58abee7102e6a6cf45ac86c41c888)

---

## 2. Modern Granular Techniques (2023-2026)

### 2.1 Synchronous vs. Asynchronous Granular

**Synchronous (Pitch-Synchronous) Granular:**
Grains are emitted at regular intervals determined by a fundamental frequency. Used for pitched synthesis, time-stretching, and formant-preserving pitch shifting. The inter-onset time equals the period of the desired pitch: `interOnsetSamples = sampleRate / desiredFrequency`.

**Asynchronous Granular:**
Grains are emitted according to stochastic distributions (Poisson, uniform, Gaussian). Controlled by density parameter (grains/second). Creates clouds, textures, and non-pitched sound masses. Inter-onset time is sampled from a probability distribution.

**Implementation Pattern:**
```
if (syncMode)
    interOnset = sampleRate / frequency;
else
    interOnset = sampleRate / density + randomJitter;
```

### 2.2 Spectral Granular Synthesis

A hybrid technique where grains are processed in the frequency domain before recombination. Each grain's DFT is computed, manipulated spectrally, then reconstructed via inverse DFT.

**Algorithm (Fasciani, 2018):**
1. Extract grain from source buffer with window
2. Compute N-point FFT of grain
3. Manipulate magnitude spectrum (frequency shifting, spectral stretching, bin selection)
4. If grain size differs from output grain size, interpolate or decimate frequency bins
5. Reconstruct phase using spectrogram inversion
6. Compute IFFT to produce output grain
7. Overlap-add output grains

**Key Advantage:** Grain pitch can be modified without time-domain resampling artifacts. Spectral characteristics are preserved independently of pitch.

**Source:** Fasciani, S. (2018). [Spectral Granular Synthesis](https://stefanofasciani.com/2018/06/07/spectral-granular-synthesis/). *Proceedings of ICMC 2018*, Daegu, Korea.

### 2.3 Wavetable-Granular Hybrids

Modern instruments combine wavetable and granular engines:
- **Waldorf Iridium:** Wavetable oscillators feeding into granular processing
- **Tasty Chips GR-MEGA:** Granular synthesis, sampling, tape mode, spectral engine in one unit; up to 5000 simultaneous grains
- **Dawesome NOVUM:** Spectral decomposition into 6 layers, each processed through independent granular engines with a "Timbre Flower" spectral editor

### 2.4 Grain-Rate Synthesis (2025)

**DataMind Audio ReFractalizer** introduces a polyphonic grain-rate synthesis engine where the grain emission rate is directly controlled by MIDI notes. This unifies BPM and Hz values, allowing interpolation between tempo-locked divisions and precise frequency values. The grain rate itself becomes a musical parameter played via keyboard.

**Source:** [DataMind Audio Refractalizer](https://synthanatomy.com/2025/11/datamind-audio-refractalizer-a-sample-morphing-and-time-bending-granular-synthesizer.html)

### 2.5 Cloud-Based Granular Control

Modern control strategies for granular clouds:

| Parameter | Low Values | High Values | Perceptual Effect |
|-----------|-----------|-------------|-------------------|
| Density | < 20 grains/sec | > 100 grains/sec | Rhythmic pulses to continuous texture |
| Spray/Jitter | 0 | Full range | Coherent to chaotic temporal spread |
| Position Randomization | 0 | Full buffer | Sequential to stochastic grain selection |
| Grain Size | 1-5 ms | 50-100 ms | Noisy/buzzy to tonal/pitched |

**Spray/Jitter:** Adds time variance to grain onset, preventing sterility and ringing artifacts from perfectly periodic grain emission. Typically implemented as uniform random offset added to calculated onset time.

**Source:** [Mutable Instruments Clouds Documentation](https://pichenettes.github.io/mutable-instruments-documentation/modules/clouds/)

---

## 3. Real-Time Implementation Strategies

### 3.1 Ross Bencina's Architecture (Definitive Reference)

**Source:** Bencina, R. (2001). ["Implementing Real-Time Granular Synthesis."](http://www.rossbencina.com/static/code/granular-synthesis/BencinaAudioAnecdotes310801.pdf) *Audio Anecdotes III*, A K Peters.

This is the single most important implementation reference for real-time granular synthesis. Bencina describes a modular architecture with three core components:

**Scheduler:**
- Maintains state for activating grains based on onset times and durations
- Manages grain allocation via a reusable pool
- Exposes a method for synthesizing samples by mixing all active grains

**SequenceStrategy:**
- Provides interface for determining when the next grain should occur and its duration
- Implementations include:
  - `DensitySequenceStrategy` (asynchronous, stochastic inter-onset times)
  - `PitchSynchronousSequenceStrategy` (regular intervals for pitched output)

**Grain:**
- Exposes interface for activation, sample synthesis, and completion query
- Contains an Envelope component and a Source component
- Source variants: `DelayLineSource`, `SampleSource`, `SyntheticSource`

**Critical architectural insight:** "Unifying grain scheduling, parameter generation, and synthesis within a Granular Synthesizer can significantly reduce overhead" compared to general-purpose event scheduling systems. Per-grain overhead in a general event system (parameter passing, object creation) becomes the bottleneck at high grain densities.

### 3.2 Voice Pool Pattern

**Pattern:** Pre-allocated fixed-size array with round-robin allocation.

```cpp
struct GrainVoice {
    bool active = false;
    float readPosition = 0.0f;
    float playbackRate = 1.0f;
    float panPosition = 0.5f;
    int samplesRemaining = 0;
    int grainLengthSamples = 0;
    bool reverse = false;
};

class GrainPool {
    static constexpr int MAX_VOICES = 64;
    std::array<GrainVoice, MAX_VOICES> voices;
    int nextVoiceIndex = 0;

    void spawnGrain(const GrainParams& p) {
        auto& v = voices[nextVoiceIndex];
        v = {};  // Reset
        v.active = true;
        // ... set parameters ...
        nextVoiceIndex = (nextVoiceIndex + 1) % MAX_VOICES;
    }
};
```

**Why round-robin:** O(1) allocation, no searching for free voices, oldest voice is always stolen first. No sorting or priority needed.

**Why NOT `juce::Synthesiser`:** JUCE's Synthesiser is designed for note-on/note-off polyphony with voice stealing based on MIDI events. Granular grains have no note-off -- they self-terminate after their duration. The overhead of MIDI parsing and voice stealing logic is unnecessary.

### 3.3 Grain Scheduling Algorithms

**Asynchronous (Stochastic) Scheduling:**
- Inter-onset time sampled from distribution
- Poisson process: `interOnset = -log(uniform_random) / density`
- Creates natural, non-periodic grain emission

**Synchronous (Periodic) Scheduling:**
- Fixed inter-onset time: `interOnset = sampleRate / frequency`
- For beat-sync: PPQ subdivision crossing detection (see O-GrainScatter research)

**Hybrid Approach:**
- Base inter-onset from density/frequency
- Add jitter: `actualOnset = baseOnset + uniform(-spray, spray)`
- spray parameter controls regularity vs chaos

### 3.4 Memory Management for Real-Time

**Rules for audio thread safety in JUCE:**
1. **Zero allocation in processBlock()** -- all buffers pre-allocated in `prepareToPlay()`
2. **No locks** -- use lock-free structures for thread communication
3. **No system calls** -- use per-instance `juce::Random`, not `getSystemRandom()`
4. **RAII only** -- no `shared_ptr` (atomic refcount causes contention)
5. **Fixed-size containers** -- `std::array` over `std::vector` on audio thread

**Typical memory budget for granular:**

| Component | Size @ 44.1kHz | Size @ 96kHz |
|-----------|----------------|--------------|
| Delay buffer (2s stereo) | ~689 KB | ~1.5 MB |
| Freeze buffer (2s stereo) | ~689 KB | ~1.5 MB |
| Grain pool (64 voices) | ~2.5 KB | ~2.5 KB |
| Total | ~1.4 MB | ~3.0 MB |

**Source:** [Real-Time Audio Processing - C++ for Audio DSP](https://oboe.com/learn/c-for-audio-dsp-1qhwprx/real-time-audio-processing-1mzn463)

---

## 4. Anti-Aliasing for Pitched Grains

### 4.1 The Aliasing Problem

When grains are pitched up (playback rate > 1.0), the effective Nyquist frequency of the source material is exceeded, causing aliasing -- frequencies above Nyquist fold back as non-harmonic distortion. This is the same problem as sample-rate conversion.

### 4.2 Interpolation Quality Hierarchy

| Method | Points | Quality | CPU Cost | Alias Suppression |
|--------|--------|---------|----------|-------------------|
| Nearest-neighbor | 1 | Poor | Minimal | None |
| Linear | 2 | Fair | Low | First sideband ~-13 dB |
| Cubic Hermite (Catmull-Rom) | 4 | Good | Medium | First sideband ~-40 dB |
| Lagrange 3rd-order | 4 | Good | Medium | Similar to cubic Hermite |
| Windowed Sinc (8-point) | 8 | Excellent | Higher | Approaches ideal |
| Windowed Sinc (16-point) | 16 | Near-ideal | High | Very high suppression |

**Practical recommendation for JUCE granular plugins:**
- **Cubic Hermite or Lagrange 3rd-order** for most use cases (4-point, good quality, reasonable cost)
- **Windowed sinc (8-16 point)** for high-quality pitch shifting where aliasing is audible
- **Linear** only acceptable for grains that are heavily enveloped and short-lived

### 4.3 Cubic Hermite Interpolation (Catmull-Rom)

```cpp
inline float hermite4(float y0, float y1, float y2, float y3, float frac) {
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}
```

### 4.4 Anti-Aliasing Strategies Beyond Interpolation

1. **Pre-filtering:** Before pitch-up, apply a low-pass filter at `nyquist / pitchRatio` to remove content that would alias. Computationally expensive per-grain.

2. **Oversampled grain playback:** Process grains at 2x-4x sample rate, then decimate with anti-alias filter. Best quality but highest CPU cost.

3. **Grain size compensation:** Larger grains have more spectral content and are more susceptible to aliasing. For high pitch ratios, reduce grain size to limit frequency content.

4. **Windowing interaction:** The grain envelope window acts as a spectral smoothing function. Gaussian and Hann windows provide better alias suppression than rectangular or triangular windows because they have lower sidelobes in the frequency domain.

### 4.5 Zero-Crossing Alignment

An alternative to heavy interpolation for avoiding discontinuities: cut grains at nearest zero crossings rather than exact time boundaries. This provides C0 continuity without explicit crossfading but does not address spectral aliasing from pitch shifting.

**Sources:**
- [Demofox Blog - Granular Audio Synthesis](https://blog.demofox.org/2018/03/05/granular-audio-synthesis/)
- [KVR - Highest Quality Realtime Sample Interpolation Methods](https://www.kvraudio.com/forum/viewtopic.php?t=501053)
- [DSP Labs - Granular Synthesis](https://lcav.gitbook.io/dsp-labs/granular-synthesis/effect_description)

---

## 5. Commercial Granular Plugins

### 5.1 Portal (Output) - $99

**Unique Approach:** Granular as an effect processor, not a synthesizer. Processes incoming audio through a granular engine with an XY Macro pad for real-time performance control.

**Key Features:**
- 250+ presets organized by instrument type and effect character
- XY Macro display pad for simultaneous control of multiple parameters
- Categories: Delay, Pitch, Stretch & Smear
- Focus on musicality over abstraction -- tempo-syncing, pitch quantization

**Architecture Insight:** Portal treats granular processing as a send effect chain. The XY pad maps to macro-controlled parameter sets, allowing complex parameter sweeps with a single gesture. This is an important UX pattern -- reducing granular's parametric complexity to gestural control.

### 5.2 Quanta 2 (Audio Damage) - $99

**Unique Approach:** Full-featured 10-voice granular synthesizer with hybrid synthesis.

**Key Features:**
- Two virtual analog oscillators that can feed the grain engine or play alongside
- Up to 100 grains per voice, 1000ms maximum grain size
- True stereo granular processing
- Subtractive and FM synthesis capabilities alongside granular
- Full modulation matrix

**Architecture Insight:** Quanta's dual-oscillator + granular engine design shows the trend toward hybrid synthesis. The oscillators can be granularized independently or used as sub-oscillators, allowing granular textures layered with stable pitched elements.

**Source:** [Quanta 2 - KVR](https://www.kvraudio.com/product/quanta-2-by-audio-damage)

### 5.3 Granulator II (Robert Henke / Ableton)

**Unique Approach:** Max for Live instrument designed for Ableton Live integration. Focus on real-time manipulation of loaded samples.

**Key Features:**
- Intuitive two-file-position grain control
- Spray (position randomization), grain size, pitch
- Designed for live performance with Ableton Push
- FilePosition and Spray as primary interaction parameters

**Architecture Insight:** Granulator II's simplicity is its strength -- only the most essential parameters are exposed. The FilePosition control (continuous scan through the sample) is the primary compositional tool, with density and grain size as secondary texture controls.

### 5.4 Borderlands (Chris Carlson / Stanford CCRMA)

**Unique Approach:** Gestural, visual, touch-based granular synthesis for iOS.

**Key Features:**
- Multi-touch grain cloud control -- drag, throw, and manipulate grain clouds
- Visual representation of grains as particles over the waveform
- Accelerometer-based control (tilt to manipulate)
- Won 2013 Prix Ars Electronica Award of Distinction

**Architecture Insight:** Borderlands demonstrates that spatial/visual interaction can replace traditional knob-based control for granular synthesis. Each grain cloud is a visual object with position (mapped to sample position), size (mapped to grain size), and behavior (mapped to density/randomization). This paradigm influenced subsequent granular plugin UI design.

**Source:** [Borderlands Paper - Chris Carlson, CCRMA](http://www.modulationindex.com/borderlands_paper.pdf)

### 5.5 Emergence (Daniel Gergely) - Free

**Unique Approach:** Real-time granular effect with 4 independent grain streams.

**Key Features:**
- Up to 600 simultaneous grains across 4 streams
- Each stream has independent parameter sets
- High-quality pitch transposition
- LFO modulation system with macros and parameter randomization
- Free (VST3 + AU)

**Architecture Insight:** The 4-stream architecture allows layering different granular textures simultaneously. Each stream can have different grain sizes, densities, and pitch settings, creating complex multi-layered textures from a single input. The 600-grain capacity suggests an efficient voice pool implementation.

**Source:** [Emergence - Daniel Gergely](https://danielgergely.net/emergence)

### 5.6 NOVUM (Dawesome / Tracktion)

**Unique Approach:** Spectral decomposition + granular resynthesis.

**Key Features:**
- Spectral import system decomposes audio into 6 distinct layers
- Each layer processed through independent granular engine
- "Timbre Flower" spectral editor for timbral manipulation
- "Homogenization" creates velvet-like textures
- SYNTIFY mode for analog-style subtractive processing of grains

**Architecture Insight:** NOVUM's 6-layer spectral decomposition is the most sophisticated approach in commercial granular plugins. By separating spectral components before granularization, each layer can be independently manipulated without affecting others. The Timbre Flower provides visual, intuitive spectral envelope control.

**Source:** [Dawesome NOVUM](https://www.dawesomemusic.com/plugins/novum/)

### 5.7 Comparative Matrix

| Plugin | Type | Max Grains | Streams/Voices | Hybrid | Unique Feature |
|--------|------|-----------|----------------|--------|----------------|
| Portal | Effect | N/A | - | No | XY Macro pad |
| Quanta 2 | Synth | 100/voice | 10 voices | FM+Sub | Dual oscillators |
| Granulator II | Synth | N/A | - | No | Ableton integration |
| Borderlands | Synth | N/A | Multi-touch | No | Gestural/visual |
| Emergence | Effect | 600 | 4 streams | No | Multi-stream architecture |
| NOVUM | Synth | N/A | 6 layers | Spectral+Sub | Spectral decomposition |
| ReFractalizer | Synth | N/A | Poly | Morphing | Grain-rate MIDI control |

---

## 6. Open-Source Implementations

### 6.1 EmissionControl2 (Curtis Roads / UCSB CREATE)

**Repository:** [github.com/EmissionControl2/EmissionControl2](https://github.com/EmissionControl2/EmissionControl2)
**Framework:** Allolib (NOT JUCE)
**Language:** C++
**Status:** Actively maintained

The definitive open-source granular synthesis application, developed under Curtis Roads at UC Santa Barbara. Features per-grain signal processing (envelope, waveform, amplitude, frequency, spatial position, filter), multi-file granulation, and real-time visualization.

**Architecture paper:** Kilgore, J., DuPlessis, R., & Roads, C. (2021). ["Architecture for Real-Time Granular Synthesis With Per-Grain Processing."](https://direct.mit.edu/comj/article-abstract/45/3/20/113899/Architecture-for-Real-Time-Granular-Synthesis-With) *Computer Music Journal*, 45(3), 20-34.

**Key Implementation Detail:** Uses Allolib multimedia library (not JUCE), but the architectural patterns (per-grain processing pipeline, parameter objects with common retrieval interface) are directly transferable.

### 6.2 GRNLR (passivist)

**Repository:** [github.com/passivist/GRNLR](https://github.com/passivist/GRNLR)
**Framework:** JUCE
**Language:** C++
**Status:** Educational resource with wiki

Open-source granular synthesis VST/AU instrument for Mac and Windows. Includes a tutorial wiki as a resource for learning JUCE-based granular synthesis development. Focus on flexibility and ease of use.

**Value:** Best starting reference for JUCE-specific granular implementation patterns due to included wiki/tutorial.

### 6.3 Argotlunar (Michael Ourednik)

**Repository:** [github.com/mourednik/argotlunar](https://github.com/mourednik/argotlunar)
**Framework:** JUCE (2013 vintage)
**Language:** C++
**Status:** Stable, no recent updates

Real-time delay-line granulator with per-grain random amplitude, panning, duration, delay, pitch, glissando, filter, and envelope. Features include:
- Feedback loop from grain output back to input
- Tempo-sync for time-related parameters
- Pitch quantization for harmonic effects
- Parameter correlation

**Value:** Most feature-complete open-source JUCE granular effect plugin. Despite the older JUCE version, the DSP architecture is sound and directly reusable.

**Source:** [Argotlunar Homepage](https://mourednik.github.io/argotlunar/)

### 6.4 GrainEngine (justlog)

**Repository:** [github.com/justlog/GrainEngine](https://github.com/justlog/GrainEngine)
**Framework:** JUCE
**Language:** C++

Based directly on Ross Bencina's article. Good reference for translating Bencina's architecture into JUCE code.

### 6.5 GranularSynth (abrigante1)

**Repository:** [github.com/abrigante1/GranularSynth](https://github.com/abrigante1/GranularSynth)
**Framework:** JUCE
**Language:** C++

Custom granular synth with documentation of the development process. Good reference for grain manager class design.

### 6.6 QuickSand (ciarandg)

**Repository:** [github.com/ciarandg/QuickSand](https://github.com/ciarandg/QuickSand)
**Framework:** JUCE
**Language:** C++

Live-input granular synthesis VST. Relevant for delay-line granular (processing live audio rather than loaded samples).

### 6.7 JR-Granular (Kengo Suzuki)

**Repository/Article:** [kengo.dev/posts/jr-granular](https://kengo.dev/posts/jr-granular)
**Framework:** JUCE + RNBO

Demonstrates exporting Cycling '74 RNBO granular patches to JUCE C++ for plugin deployment. Useful reference for hybrid Max/JUCE workflows.

### 6.8 STK Granulate (CCRMA)

**Repository:** [Synthesis ToolKit in C++](https://ccrma.stanford.edu/software/stk/classstk_1_1Granulate.html)
**Framework:** STK (standalone)
**Language:** C++

Stanford's STK provides a `Granulate` class implementing granular synthesis on input soundfiles. Clean, well-documented API with envelope control, voice management, and grain parameter functions.

### 6.9 Grainstation-C

**Repository:** Open source, supports ambisonics
**Language:** C++

Free and open-source granular workstation with live-playable interface and ambisonic spatial sound support. Notable for combining granular synthesis with spatial audio in an open-source context.

**Source:** [CDM - Grainstation-C](https://cdm.link/2019/07/grainstation-free-granular-tool-ambient-album/)

---

## 7. Advanced Grain Envelope Shapes

### 7.1 Overview

The grain envelope (window function) serves two purposes:
1. **Temporal:** Smooth grain onset/offset to prevent clicks
2. **Spectral:** Shape the spectral characteristics of each grain (lower sidelobes = less spectral leakage)

### 7.2 Envelope Formulas

**Hann (Raised Cosine):**
```
w(n) = 0.5 * (1 - cos(2*pi*n / (N-1)))
```
- Most common choice for general-purpose granular synthesis
- Zero at endpoints (no clicks)
- First sidelobe: -31.5 dB
- Implementation: single `cos()` call per sample

**Gaussian:**
```
w(n) = exp(-0.5 * ((n - (N-1)/2) / (sigma * (N-1)/2))^2)
```
- Bell-shaped, does NOT reach zero at endpoints
- Sigma parameter controls width: low sigma (~0.1) = narrow/choppy, high sigma (~0.5) = wide/smooth
- **Problem:** For sigma > 0.3, the window does not reach zero, causing clicks
- **Solution:** Multiply by a trapezoidal window to force zero endpoints
- Best spectral properties of simple windows (minimal spectral smearing)
- Preferred in analysis/resynthesis applications

**Hamming:**
```
w(n) = 0.54 - 0.46 * cos(2*pi*n / (N-1))
```
- Similar to Hann but raised so bottom does not touch zero
- First sidelobe: -42.7 dB (better than Hann)
- Slight discontinuity at endpoints (0.08, not 0.0)

**Blackman:**
```
w(n) = 0.42 - 0.5 * cos(2*pi*n/(N-1)) + 0.08 * cos(4*pi*n/(N-1))
```
- Three-term cosine sum
- First sidelobe: -58.1 dB (excellent)
- Wider main lobe than Hann (more spectral smearing)
- Good for analysis/resynthesis where sidelobe suppression matters more than frequency resolution

**Trapezoidal:**
```
w(n) = { n/rampLength,           for n < rampLength
       { 1.0,                    for rampLength <= n <= N-rampLength
       { (N-1-n)/rampLength,     for n > N-rampLength
```
- Linear attack, sustain, linear release
- `rampPercent` parameter: 0% = rectangular, 50% = trapezoidal, 100% = triangular
- Most computationally efficient (no trig functions)
- Sharp spectral transitions due to linear segments
- Common in efficiency-critical implementations

**Triangular:**
```
w(n) = 1 - |2*n/(N-1) - 1|
```
- Special case of trapezoid with rampPercent = 100%
- Reaches zero at endpoints
- Simple to compute

**Tukey (Tapered Cosine):**
```
w(n) = { 0.5*(1 + cos(pi*(2*n/(alpha*(N-1)) - 1))),    for 0 <= n < alpha*(N-1)/2
       { 1.0,                                             for alpha*(N-1)/2 <= n <= (N-1)*(1-alpha/2)
       { 0.5*(1 + cos(pi*(2*n/(alpha*(N-1)) - 2/alpha + 1))),  for rest
```
- Alpha parameter: 0 = rectangular, 1 = Hann
- Combines flat top (preserving grain amplitude) with cosine tapers
- Good compromise between temporal and spectral properties

### 7.3 Asymmetric Envelopes

Envelopes need not be symmetrical. An asymmetric envelope with short attack and long decay (or vice versa) creates different timbral results:
- **Short attack + long decay:** Percussive grain character, emphasizes transients
- **Long attack + short decay:** Reverse-swell character, softer onset
- **Implementation:** Use independent attack and release ramp lengths in the trapezoidal formula, or skew the phase of a Hann window

### 7.4 Complex Envelopes (Grainlet Envelopes)

Two-layer envelopes where a sinusoidal carrier is modulated by a second envelope:
```
w(n) = carrier(n) * modulator(n)
```
This connects to wavelet transform analysis and creates more complex spectral shapes per grain.

### 7.5 Parabolic Envelope (Bencina)

Bencina identifies the parabolic envelope as equally efficient to trapezoidal and raised cosine:
```
w(n) = 1 - (2*n/(N-1) - 1)^2
```
- Smooth, zero at endpoints
- No trig functions needed
- Can be computed incrementally using second-order differences

### 7.6 Compute vs. Lookup Table

**Modern recommendation: Compute per-sample, not lookup table.**

Rationale:
- `cos()` costs ~3-10 cycles with SIMD on modern CPUs
- A large LUT for variable-length grains either wastes cache (large table) or requires interpolation (adding complexity)
- With 64 voices max: 64 `cos()` calls per sample = ~640 cycles = negligible at 44.1kHz
- Cache pressure from LUT evicts useful audio data from L1/L2

**Sources:**
- [Granular Synthesis Window Functions](https://michaelkrzyzaniak.com/AudioSynthesis/2_Audio_Synthesis/11_Granular_Synthesis/1_Window_Functions/)
- [Sound In A Nutshell - Grain Shapes](https://www.granularsynthesis.com/hthesis/shape.html)
- [ResearchGate - Grain Windowing Shapes](https://www.researchgate.net/figure/Grain-Windowing-Shapes-a-Gaussian-b-Cosine-c-Triangle-d-Rectangle-e_fig5_259044124)

---

## 8. Time-Stretching and Pitch-Shifting Algorithms

### 8.1 Algorithm Comparison

| Algorithm | Domain | Time Stretch | Pitch Shift | Polyphonic | Latency | Artifacts |
|-----------|--------|-------------|-------------|------------|---------|-----------|
| OLA | Time | Yes | No* | Fair | Low | Phasing |
| WSOLA | Time | Yes | No* | Poor | Low | Transient doubling |
| TD-PSOLA | Time | Yes | Yes | Poor | Low | Requires F0 detection |
| Phase Vocoder | Freq | Yes | Yes | Good | Medium-High | Phasiness, transient smearing |
| Granular | Time | Yes | Yes | Fair | Low | Texture artifacts |
| PVSOLA | Hybrid | Yes | Yes | Good | Medium | Best combined quality |

*Pitch shift achieved by combining time-stretch with resampling.

### 8.2 OLA (Overlap-Add) -- Baseline

The simplest approach: extract overlapping frames, space them differently in time.
- **Time stretch:** Change the hop size between output frames
- **Problem:** Phase misalignment between overlapping frames causes constructive/destructive interference (phasing artifacts)

### 8.3 WSOLA (Waveform Similarity Overlap-Add)

Improvement over OLA: allows timing tolerance in analysis frame placement to maximize waveform similarity via cross-correlation.

```
bestOffset = argmax(cross_correlation(frame[n], frame[n+1], tolerance))
```

- **Strength:** Works well for monophonic, pitched sources
- **Weakness:** Cannot properly handle polyphonic material -- different frequency components need different alignments
- **Artifact:** "Transient doubling/stuttering" -- transients may be repeated or smeared

### 8.4 TD-PSOLA (Time-Domain Pitch-Synchronous Overlap-Add)

Pitch-synchronous analysis: frames are placed at pitch periods. Can independently modify pitch and duration.
- **Requirement:** Needs F0 (fundamental frequency) detection
- **Strength:** High quality for monophonic pitched audio (vocals)
- **Weakness:** Fails for unpitched or polyphonic material

### 8.5 Phase Vocoder

Frequency-domain approach using STFT:
1. STFT analysis with overlapping windows
2. Modify phase relationships to achieve time-stretching
3. Optional pitch-shifting via bin shifting
4. ISTFT resynthesis

- **Strength:** Works with polyphonic and complex material
- **Weakness:** "Phasiness" -- smeared transients, metallic quality at extreme stretching
- **Improvement: PVSOLA** -- Phase Vocoder with Synchronized Overlap-Add, combining phase vocoder's frequency-domain processing with time-domain waveform alignment

### 8.6 Granular Approach

Granular time-stretching: play back grains from the source at modified positions:
- **Time stretch:** Scan through source at modified rate while maintaining grain size and density
- **Pitch shift:** Modify grain playback rate: `rate = 2^(semitones/12)`
- **Combined:** Independent control of time and pitch

**Advantages over phase vocoder:**
- Lower latency (no FFT window size constraint)
- No phase coherence issues
- More natural-sounding at extreme time stretches (texture rather than smearing)
- Simpler implementation

**Disadvantages:**
- Less transparent for moderate stretching (audible granularity)
- No formant preservation without additional processing
- Grain size must be tuned to source material

### 8.7 Practical Recommendation for JUCE Plugin

For a granular plugin that needs pitch shifting:
1. **Primary:** Granular playback rate modification with quality interpolation (Lagrange 3rd or cubic Hermite)
2. **For transparency:** Add optional per-grain anti-alias filtering for pitch-up
3. **For extreme stretch:** Large grains (50-100ms) with high overlap (75%+) and Hann windowing

**Sources:**
- [Stephan Bernsee - Time Stretching and Pitch Shifting Overview](http://blogs.zynaptiq.com/bernsee/time-pitch-overview/)
- [PVSOLA Paper (IRCAM DAFx 2011)](http://recherche.ircam.fr/pub/dafx11/Papers/57_e.pdf)
- [Improved PVSOLA (DAFx 2012)](https://www.dafx12.york.ac.uk/papers/dafx12_submission_26.pdf)
- [PyTSMod - Python TSM Implementations](https://pypi.org/project/pytsmod/)

---

## 9. Spatial and Multichannel Granular

### 9.1 Per-Grain Spatialization

Each grain can be independently spatialized, creating a distributed sound field:

```cpp
struct SpatialGrainVoice {
    // ... standard grain params ...
    float azimuth;    // Horizontal angle (-180 to 180)
    float elevation;  // Vertical angle (-90 to 90)
    float distance;   // Distance from listener
};
```

**Stereo panning (simplest):**
```cpp
float panL = std::cos(panPosition * M_PI * 0.5f);
float panR = std::sin(panPosition * M_PI * 0.5f);
outputL += grainSample * panL * envelope;
outputR += grainSample * panR * envelope;
```

### 9.2 Ambisonics for Granular

**Research:** Rossetti et al. (2020) studied the perception of granular sounds spatialized in high-order ambisonics systems. Key findings:
- Grain size and rarefaction rate significantly affect spatial perception
- Smaller grains (20ms) with high rarefaction (0.9) create discontinuous spatial fields
- Larger grains (300ms) produce continuous spatial textures
- Higher ambisonics orders (9th) yield richer spatial experiences than lower orders
- Each grain has a short, specific temporary location in the multichannel system

**Implementation with Ambitools:**
Ambitools generates swarms of spatialized sound grains in a spherical shell sector using Higher Order Ambisonic format. Each grain is encoded into the ambisonics domain at its designated spatial position, then decoded for the speaker configuration.

**Source:** [Spatial Granular Synthesis With Ambitools and Supercollider (FAUST IFC 2024)](https://faust.grame.fr/community/ifc/2024/Spatial_Granular_Synthesis_With_Ambitools_And_Supercollider.pdf)

### 9.3 JUCE Implementation Considerations

For JUCE plugins targeting spatial granular:

1. **Stereo:** Simple equal-power panning per grain (baseline, works everywhere)
2. **Binaural:** HRTF-based spatialization per grain using convolution with compact HRIR filters
3. **Ambisonics:** Encode each grain to B-format, output ambisonics channels
4. **Surround (5.1/7.1):** VBAP (Vector Base Amplitude Panning) per grain

**Channel layout in JUCE:**
```cpp
// In getChannelLayoutOfBus():
// Support multiple layouts
return juce::AudioChannelSet::stereo()       // 2ch
    || juce::AudioChannelSet::quadraphonic() // 4ch
    || juce::AudioChannelSet::create5point1() // 6ch
    || juce::AudioChannelSet::create7point1() // 8ch
    || juce::AudioChannelSet::ambisonic(3);   // HOA 3rd order (16ch)
```

### 9.4 Research Papers

- Rossetti, D., et al. (2020). ["Studying the Perception of Sound in Space: Granular Sounds Spatialized in a High-Order Ambisonics System."](https://www.anppom.com.br/revista/index.php/opus/article/view/opus2020b2610/0) *OPUS*, 26(1).
- ["Creating Timbre in Space: Granular Synthesis and Ambisonics Spatialization Study and Composition"](https://www.researchgate.net/publication/333934109)
- ["Granular Spatialisation: A New Method for Sound Diffusion"](https://quod.lib.umich.edu/cgi/p/pod/dod-idx/granular-spatialisation-a-new-method-for-sound-diffusion.pdf?c=icmc&idno=bbp2372.2016.006&format=pdf) (ICMC 2016)
- ["Spatial Granular Synthesis With Ambitools and AnteScollider"](https://hal.science/hal-04846653v1/document) (HAL, 2024)

---

## 10. Performance Optimization

### 10.1 SIMD for Grain Processing

SIMD (Single Instruction Multiple Data) can process 4 (SSE) or 8 (AVX) float operations simultaneously. For granular synthesis, SIMD can be applied to:

**Grain mixing:** When summing multiple grains to the output buffer:
```cpp
// Process 4 grains at once using SSE
__m128 grainSamples = _mm_set_ps(grain3, grain2, grain1, grain0);
__m128 envelopes = _mm_set_ps(env3, env2, env1, env0);
__m128 result = _mm_mul_ps(grainSamples, envelopes);
// Horizontal sum for mono output
```

**Block processing per grain:** Process a grain's output for an entire buffer block, then mix:
```cpp
// Process grain i for entire block
for (int s = 0; s < numSamples; s += 4) {
    __m128 samples = /* read 4 interpolated samples */;
    __m128 envelope = /* compute 4 envelope values */;
    __m128 output = _mm_mul_ps(samples, envelope);
    __m128 existing = _mm_load_ps(&outputBuffer[s]);
    _mm_store_ps(&outputBuffer[s], _mm_add_ps(existing, output));
}
```

**JUCE SIMD support:** JUCE provides `juce::dsp::SIMDRegister<float>` for cross-platform SIMD, but for granular synthesis the per-grain structure often doesn't align well with SIMD's requirement for contiguous parallel data. Manual SSE/AVX intrinsics or restructuring to SoA (Structure of Arrays) layout may be more effective.

### 10.2 Cache-Friendly Data Layouts

**Structure of Arrays (SoA) vs Array of Structures (AoS):**

```cpp
// AoS (typical, cache-unfriendly for iteration):
struct Grain { float pos, rate, pan; int remaining; bool active; };
std::array<Grain, 64> grains;

// SoA (cache-friendly for per-field iteration):
struct GrainPool {
    std::array<float, 64> positions;
    std::array<float, 64> rates;
    std::array<float, 64> pans;
    std::array<int, 64> remaining;
    std::array<bool, 64> active;
};
```

SoA layout ensures that when iterating over all grain positions (for example), all position data is contiguous in memory, maximizing cache line utilization. However, if the inner loop processes all fields of one grain before moving to the next (which is the typical granular processing pattern), AoS may be better because all fields of one grain fit in a single cache line.

**Practical recommendation:** For 64 grains, the entire AoS array is ~2.5 KB -- well within L1 cache (typically 32-64 KB). Cache optimization becomes important only with very high grain counts (500+). Below that, code clarity should take priority.

### 10.3 Lock-Free Communication

**SPSC Ring Buffer** for parameter updates from UI to audio thread:

```cpp
// UI thread writes parameter changes:
ringBuffer.push({paramId, newValue});

// Audio thread reads at block start:
while (ringBuffer.pop(update))
    applyParameterUpdate(update);
```

**Cache line alignment** to prevent false sharing:
```cpp
struct alignas(64) AtomicIndex {
    std::atomic<size_t> value{0};
    char padding[64 - sizeof(std::atomic<size_t>)];
};
```

For JUCE plugins, `APVTS::getRawParameterValue()` returns `std::atomic<float>*` which is already lock-free. Custom ring buffers are only needed for non-parameter data (waveform display, grain activity visualization).

### 10.4 Buffer Size and Block Processing

**Fixed-size block processing** improves determinism:
- Pre-allocate all temporary buffers for the maximum expected block size
- Avoid variable-length allocations based on host buffer size
- Process grains in blocks rather than per-sample when possible (better SIMD utilization)

### 10.5 Profiling Priorities

Typical CPU distribution in a granular engine (64 voices, Hann window, Lagrange3 interpolation):

| Component | Estimated CPU % | Notes |
|-----------|----------------|-------|
| Sample interpolation (4-point) | 35-45% | 64 voices x 4 multiplies + adds per sample |
| Envelope computation | 10-15% | 64 `cos()` calls per sample |
| Buffer read (memory access) | 15-25% | Random access pattern, cache-dependent |
| Grain scheduling | 5-10% | Per-block, not per-sample |
| Mixing and panning | 5-10% | Linear operations |
| Parameter smoothing | 2-5% | SmoothedValue per-sample |

**Optimization priority order:**
1. Interpolation quality/cost trade-off (biggest impact)
2. Memory access patterns (buffer layout, prefetching)
3. Envelope computation method (compute vs. LUT trade-off)
4. SIMD for mixing/panning (only after profiling confirms bottleneck)

### 10.6 GPU-Accelerated Granular

**CudaGrain** demonstrates CUDA-based granular synthesis, using 2D textures where each row represents one grain. Parameters are set per grain (frequency, amplitude, attack, decay, lifetime, waveform shape) and a GPU shader computes the output.

**Current state:** GPU granular remains experimental for audio plugins. The CPU-to-GPU transfer latency typically exceeds the processing time savings for typical grain counts (< 1000). GPU acceleration becomes beneficial only for extreme grain counts (5000+) or per-grain spectral processing (FFT per grain).

**Recommendation:** For JUCE plugins, CPU-based processing with SIMD optimization is the practical choice. GPU acceleration is only relevant for standalone applications or very high grain count scenarios.

**Sources:**
- [SIMD in DSP - WolfSound](https://thewolfsound.com/simd-in-dsp/)
- [Optimizing DSP for Real-Time Audio](https://www.numberanalytics.com/blog/optimizing-dsp-real-time-audio-processing)
- [Optimizing Ring Buffer for Throughput](https://rigtorp.se/ringbuffer/)
- [CudaGrain](https://sites.google.com/site/cudagrain/technology)

---

## Summary of Key Recommendations for JUCE Implementation

### Architecture
- Use Ross Bencina's Scheduler/SequenceStrategy/Grain pattern
- Pre-allocate fixed-size voice pool (64-128 voices) with round-robin allocation
- Header-only DSP classes for inlining in the per-sample loop
- Manual circular buffer (not `juce::dsp::DelayLine`) for multi-tap grain reading

### Interpolation
- Lagrange 3rd-order or Cubic Hermite for grain playback (4-point, good quality/cost ratio)
- Windowed sinc only if aliasing is audibly problematic at high pitch ratios

### Envelopes
- Hann window as default (compute per-sample, not LUT)
- Offer Gaussian, trapezoidal, Tukey as options for different timbral characters
- Consider asymmetric envelopes for percussive/reverse character

### Scheduling
- Poisson-distributed inter-onset for asynchronous mode
- PPQ subdivision crossing for beat-sync mode
- Spray/jitter parameter for controlled randomization of onset times

### Memory
- Zero allocation in processBlock
- All buffers pre-allocated in prepareToPlay
- Per-instance `juce::Random` (not system random)

### Optimization
- Profile first, optimize second
- Cache locality matters more than SIMD for typical grain counts
- AoS layout is fine for <= 128 grains (fits in L1 cache)
- SoA layout beneficial only for 500+ grains with SIMD processing

---

## References (Complete)

### Books
- Gabor, D. (1947). "Acoustical Quanta and the Theory of Hearing." *Nature*, 159(4044).
- Xenakis, I. (1971). *Formalized Music*. Indiana University Press.
- Roads, C. (1996). *The Computer Music Tutorial*. MIT Press.
- Roads, C. (2001). *Microsound*. MIT Press. ISBN: 978-0262681544.
- Truax, B. (1988). "Real-time granular synthesis with a digital signal processing computer." *Computer Music Journal*, 12(2).

### Papers
- Bencina, R. (2001). ["Implementing Real-Time Granular Synthesis."](http://www.rossbencina.com/static/code/granular-synthesis/BencinaAudioAnecdotes310801.pdf) *Audio Anecdotes III*.
- Fasciani, S. (2018). ["Spectral Granular Synthesis."](https://stefanofasciani.com/2018/06/07/spectral-granular-synthesis/) *Proceedings of ICMC 2018*.
- Kilgore, J., DuPlessis, R., Roads, C. (2021). ["Architecture for Real-Time Granular Synthesis With Per-Grain Processing."](https://direct.mit.edu/comj/article-abstract/45/3/20/113899/) *Computer Music Journal*, 45(3).
- Rossetti, D. et al. (2020). ["Studying the Perception of Sound in Space."](https://www.anppom.com.br/revista/index.php/opus/article/view/opus2020b2610/0) *OPUS*, 26(1).
- ["Granular Spatialisation."](https://quod.lib.umich.edu/cgi/p/pod/dod-idx/granular-spatialisation-a-new-method-for-sound-diffusion.pdf?c=icmc&idno=bbp2372.2016.006&format=pdf) ICMC 2016.
- [PVSOLA Paper (IRCAM DAFx 2011)](http://recherche.ircam.fr/pub/dafx11/Papers/57_e.pdf)

### Web Resources
- [Barry Truax - Granular Synthesis](https://www.sfu.ca/~truax/gran.html)
- [Iannis Xenakis - Granular Synthesis](https://www.iannis-xenakis.org/en/granular-synthesis/)
- [Josh Stovall - Granular Synthesis Overview](https://joshstovall.com/writing/granular-synthesis/)
- [Demofox Blog - Granular Audio Synthesis](https://blog.demofox.org/2018/03/05/granular-audio-synthesis/)
- [WolfSound - SIMD in DSP](https://thewolfsound.com/simd-in-dsp/)
- [Erik Rigtorp - Optimizing Ring Buffer](https://rigtorp.se/ringbuffer/)
- [KVR Forum - Open Source Granular Synthesis Code](https://www.kvraudio.com/forum/viewtopic.php?t=537482)

### Open-Source Repositories
- [EmissionControl2](https://github.com/EmissionControl2/EmissionControl2) (C++/Allolib)
- [GRNLR](https://github.com/passivist/GRNLR) (JUCE)
- [Argotlunar](https://github.com/mourednik/argotlunar) (JUCE)
- [GrainEngine](https://github.com/justlog/GrainEngine) (JUCE)
- [GranularSynth](https://github.com/abrigante1/GranularSynth) (JUCE)
- [QuickSand](https://github.com/ciarandg/QuickSand) (JUCE)
- [STK Granulate](https://ccrma.stanford.edu/software/stk/classstk_1_1Granulate.html) (C++)

### Commercial Plugins Analyzed
- [Portal (Output)](https://output.com/products/portal)
- [Quanta 2 (Audio Damage)](https://www.kvraudio.com/product/quanta-2-by-audio-damage)
- [Emergence (Daniel Gergely)](https://danielgergely.net/emergence)
- [Borderlands (Chris Carlson)](http://www.borderlands-granular.com/app/)
- [NOVUM (Dawesome/Tracktion)](https://www.dawesomemusic.com/plugins/novum/)
- [ReFractalizer (DataMind Audio)](https://synthanatomy.com/2025/11/datamind-audio-refractalizer-a-sample-morphing-and-time-bending-granular-synthesizer.html)
