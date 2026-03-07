---
title: "Concatenative Synthesis: Comprehensive Technical Reference"
created: 2026-02-08
domain: dsp
type: research
keywords:
  - concatenative-synthesis
  - corpus-based
  - audio-analysis
  - feature-extraction
  - granular
---
# Concatenative Synthesis: Comprehensive Technical Reference

## Table of Contents
1. [What is Concatenative Synthesis?](#1-what-is-concatenative-synthesis)
2. [History and Origins](#2-history-and-origins)
3. [Core Algorithm](#3-core-algorithm)
4. [Key Audio Descriptors](#4-key-audio-descriptors)
5. [Real-Time vs Offline](#5-real-time-vs-offline)
6. [Notable Implementations](#6-notable-implementations)
7. [Relationship to Corpus-Based Synthesis](#7-relationship-to-corpus-based-synthesis)
8. [C++/JUCE Implementation Considerations](#8-cjuce-implementation-considerations)

---

## 1. What is Concatenative Synthesis?

### Definition

Concatenative synthesis is a technique for synthesizing sounds by concatenating (stringing together) short samples of recorded sound called **units**. A large database of source sounds (a **corpus**) is segmented into units, analyzed with audio descriptors, and a **unit selection algorithm** finds the units that best match a target specification. The selected units are then transformed and concatenated to produce the output.

Unit durations typically range from **10ms to 1 second**, placing them between the micro-scale of granular synthesis and the macro-scale of sample-based instruments.

### How It Differs from Other Synthesis Methods

| Synthesis Method | Core Principle | Key Difference from Concatenative |
|---|---|---|
| **Subtractive** | Filter harmonically-rich waveforms | Generates sound from oscillators, not recorded audio |
| **Additive** | Sum sine waves to build complex tones | Constructs from mathematical primitives, not audio segments |
| **Wavetable** | Morph between stored single-cycle waveforms | Uses single cycles, not variable-length audio segments |
| **Granular** | Plays tiny grains (1-100ms) from audio | Similar but grains are selected by position/parameter, NOT by descriptor matching |
| **FM** | Modulate frequency of one oscillator with another | Purely mathematical, no audio database |
| **Physical Modeling** | Simulate physical vibrating systems | Mathematical model, no audio database |
| **Concatenative** | Select and join audio units by descriptor matching | Uses MIR (Music Information Retrieval) to intelligently select segments |

### Concatenative vs Granular: The Critical Distinction

While concatenative synthesis is closely related to granular synthesis, the fundamental difference is the **selection mechanism**:

- **Granular synthesis**: Grains are selected based on **position** within a source file, with parameters like position, density, pitch, and spray controlled directly by the user or modulation sources. The system has no "understanding" of the audio content.

- **Concatenative synthesis**: Units are selected based on **audio descriptor matching** using MIR technology. The system analyzes what the audio *sounds like* (pitch, timbre, loudness, spectral content) and selects units based on proximity in a multi-dimensional descriptor space to a target specification.

Concatenative synthesis can be thought of as a **content-based extension to granular synthesis** -- providing direct access to specific sound characteristics rather than just temporal position.

---

## 2. History and Origins

### Timeline

**1950s-1980s: Precursors**
- Early electronic music composers manually spliced tape recordings -- a physical form of concatenation.
- **Diphone synthesis** emerged in the 1980s for speech, using small databases of phone-pair transitions.

**1996: The Foundational Paper**
- **Andrew Hunt and Alan Black** published "Unit Selection in a Concatenative Speech Synthesis System Using a Large Speech Database" (ICASSP 1996). This seminal paper formalized the **unit selection** approach with target cost and concatenation cost functions, and the use of the **Viterbi algorithm** for optimal path selection through a trellis of candidate units.
- This was a paradigm shift from fixed-unit diphone synthesis to large-corpus unit selection.

**2000: Extension to Music**
- **Diemo Schwarz** (IRCAM) published "A System for Data-Driven Concatenative Sound Synthesis" (DAFx 2000), extending concatenative techniques from speech to musical sound synthesis.

**2005-2006: CataRT and Survey**
- **CataRT** (Schwarz, Beller, Verbrugghe, Britton) released as a real-time corpus-based concatenative synthesis system at IRCAM.
- Schwarz published the comprehensive survey "Concatenative Sound Synthesis: The Early Years" in the Journal of New Music Research (2006, Vol 35, No 1, pp 3-22), providing a taxonomy of all approaches.

**2006-2015: Proliferation**
- **AudioGuide** by Ben Hackbarth et al. -- offline concatenative synthesis with hierarchical search.
- **C-C-Combine** by Rodrigo Constanzo -- accessible real-time implementation in Max/MSP.
- Various academic systems for audio mosaicing / musaicing.

**2018-Present: Commercial Products and AI Integration**
- **AudioTexture** by Le Sound -- commercial VST/AU/AAX plugin.
- **Catecophony** -- open-source VST3/AU real-time implementation using JUCE and Essentia.
- **The Concatenator** by DataMind Audio (2024-2025) -- commercial plugin using Bayesian particle filtering.
- Integration with neural networks and machine learning for improved feature matching.

### Key Researchers

| Researcher | Affiliation | Contribution |
|---|---|---|
| **Diemo Schwarz** | IRCAM, Paris | CataRT, foundational surveys, corpus-based concatenative synthesis theory |
| **Andrew Hunt** | University of Edinburgh | Unit selection speech synthesis (1996 foundational paper) |
| **Alan Black** | CMU / University of Edinburgh | Unit selection cost functions, Viterbi-based selection |
| **Ben Hackbarth** | Various | AudioGuide offline concatenative synthesis |
| **Norbert Schnell** | IRCAM | AudioGuide, real-time audio processing |
| **Rodrigo Constanzo** | Independent | C-C-Combine, accessible real-time implementation |
| **Chris Tralie** | Ursinus College | The Concatenator, Bayesian real-time musaicing (ISMIR 2024) |

### Key Papers

1. Hunt, A. & Black, A. (1996). "Unit Selection in a Concatenative Speech Synthesis System Using a Large Speech Database." ICASSP.
2. Schwarz, D. (2000). "A System for Data-Driven Concatenative Sound Synthesis." DAFx 2000.
3. Schwarz, D. (2006). "Concatenative Sound Synthesis: The Early Years." Journal of New Music Research, 35(1), 3-22.
4. Schwarz, D., Beller, G., Verbrugghe, B., Britton, S. (2006). "Real-Time Corpus-Based Concatenative Synthesis with CataRT." DAFx 2006.
5. Tralie, C. & Cantil, B. (2024). "The Concatenator: A Bayesian Approach to Real Time Concatenative Musaicing." ISMIR 2024.

---

## 3. Core Algorithm

The concatenative synthesis pipeline has four major stages:

### Stage 1: Corpus Construction (Offline)

#### 1a. Audio Ingestion
Load one or more source audio files into the system. The corpus can contain anything: field recordings, instrument samples, vocal recordings, synthesized sounds, etc.

#### 1b. Segmentation
Divide the audio into discrete **units**. Segmentation strategies include:

- **Fixed-size segments**: Divide into equal-length windows (e.g., every 50ms, 100ms, 200ms). Simplest approach.
- **Onset/transient detection**: Segment at detected onsets using high-frequency content (HFC) analysis or spectral flux. Produces musically meaningful units (notes, hits).
- **Silence-based splitting**: Detect silence thresholds and split at quiet gaps.
- **Beat-synchronous**: Align segments to detected beats/tempo.
- **Pitch-based**: Segment at pitch changes or stable pitch regions.
- **Arbitrary/manual**: User-defined segment boundaries.

Each unit stores: `{source_file_index, start_sample, duration_samples, descriptor_vector}`.

#### 1c. Feature Extraction (Descriptor Analysis)
For each unit, compute a vector of audio descriptors (see Section 4). CataRT computes approximately **230 MPEG-7 descriptors** across four categories, though a practical system typically uses 5-20 key descriptors.

Feature extraction is performed per-frame (typically 1024-4096 sample windows with 50-75% overlap), then statistics (mean, variance, min, max) are computed across each unit.

#### 1d. Index Construction
Build a spatial index for efficient nearest-neighbor lookup:
- **KD-tree**: Most common. Recursively partitions descriptor space along principal component vectors. Provides O(log n) average lookup time.
- **Ball tree**: Alternative for high-dimensional spaces.
- **VP-tree**: Vantage-point tree for metric spaces.
- **Flat search**: For small corpora, brute-force Euclidean distance can be fast enough.

### Stage 2: Target Specification

The target defines what the output should sound like. Methods include:

#### 2a. Audio-Guided (Audio Mosaicing / Musaicing)
A target audio file or live audio stream is analyzed frame-by-frame using the same descriptors as the corpus. Each target frame becomes a query point in descriptor space. This is the most common approach for real-time use.

#### 2b. Manual/Interactive Navigation
The user directly navigates a 2D or 3D projection of the descriptor space (e.g., via mouse, MIDI controller, or touchscreen). CataRT's primary interface uses this approach.

#### 2c. Sequencer-Driven
A target sequence of descriptor values is specified programmatically or via a timeline/sequencer.

#### 2d. Descriptor Transformation
Apply transformations to the target descriptors (scaling, offsetting, mapping) to create creative remappings between target and corpus spaces.

### Stage 3: Unit Selection

#### 3a. The Cost Function Formulation

The classic formulation (Hunt & Black, 1996) defines two cost functions:

**Target Cost** -- measures how well a candidate unit u_i matches the target specification t_i:

```
C_target(t_i, u_i) = SUM_j(w_j * C_j(t_i, u_i))
```

Where:
- `w_j` = weight for descriptor j
- `C_j` = sub-cost for descriptor j (typically squared Euclidean distance for continuous descriptors, or binary match/mismatch for categorical descriptors)

**Concatenation Cost** (Join Cost) -- measures the acoustic discontinuity at the boundary between consecutive selected units:

```
C_concat(u_{i-1}, u_i) = SUM_j(w_j * C_j(u_{i-1}, u_i))
```

This evaluates spectral/cepstral continuity, power continuity, and pitch continuity at join points.

**Total Cost** for selecting a sequence of units u_1...u_n given target sequence t_1...t_n:

```
C_total = SUM_{i=1}^{n} C_target(t_i, u_i) + SUM_{i=2}^{n} C_concat(u_{i-1}, u_i)
```

#### 3b. Optimal Sequence Selection: Viterbi Algorithm

The unit selection problem forms a **trellis** where:
- Each column represents a target frame position
- Each row represents a candidate unit from the corpus
- Edge weights encode target cost (node cost) + concatenation cost (transition cost)

The **Viterbi algorithm** (dynamic programming) finds the minimum-cost path through this trellis in O(n * m^2) time, where n = number of target frames and m = number of candidate units.

**Beam search** pruning limits m to the top-k candidates per frame for tractability with large corpora.

#### 3c. Simplified Real-Time Selection: Nearest Neighbor

For real-time systems, the full Viterbi search is often replaced with a simpler approach:

1. For each target frame, query the KD-tree for the **k-nearest neighbors** in descriptor space.
2. Select either:
   - The **closest** match (greedy, lowest latency)
   - A **random selection** from units within a distance threshold (adds variation)
   - A **weighted random** selection considering both target cost and concatenation cost with the previous unit

This sacrifices global optimality for O(log n) per-frame lookup time.

#### 3d. Bayesian/Particle Filter Approach (The Concatenator, 2024)

Treats corpus window indices as **hidden states** and target audio as **observations**. Uses a **particle filter** to maintain multiple hypotheses about which corpus regions best explain the incoming audio, with:
- **Transition probability**: Controls how likely the system is to stay on the current corpus segment vs. jump
- **Observation likelihood**: How well a corpus window matches the current target frame
- **Particle count**: Number of parallel hypotheses (controls accuracy vs. CPU)

This approach's computational complexity is **independent of corpus size** (depends only on particle count), enabling scaling to hours-long corpora.

### Stage 4: Concatenation and Output

#### 4a. Crossfading
Apply crossfade envelopes at unit boundaries to smooth transitions:
- **Linear crossfade**: Simple but may produce amplitude dips
- **Equal-power crossfade**: `sqrt()` curves maintain constant power
- **Raised cosine (Hann)**: Good general-purpose crossfade shape
- Typical crossfade length: 5-50ms

#### 4b. Overlap-Add (OLA)
For smoother results, use overlap-add:
- Adjacent units overlap by the crossfade duration
- Each unit has fade-in and fade-out envelopes applied
- Overlapping samples are summed

#### 4c. PSOLA (Pitch-Synchronous Overlap-Add)
For pitch-sensitive material, PSOLA aligns crossfade points to pitch periods (glottal pulses in speech, fundamental period in music), dramatically reducing artifacts.

#### 4d. Post-Processing
- **Gain normalization**: Match loudness across units
- **Pitch correction**: Micro-adjust pitch at boundaries via resampling
- **Spectral smoothing**: Apply spectral envelope interpolation at join points

---

## 4. Key Audio Descriptors

### Descriptor Categories

Audio descriptors are the foundation of concatenative synthesis. They characterize "what the audio sounds like" in numerical form.

#### Temporal/Energy Descriptors

| Descriptor | Description | Computation | Use in Concatenative Synthesis |
|---|---|---|---|
| **RMS Energy** | Root mean square amplitude | `sqrt(mean(x^2))` | Match loudness levels |
| **Peak Amplitude** | Maximum absolute sample value | `max(abs(x))` | Dynamic matching |
| **Zero-Crossing Rate (ZCR)** | Rate of sign changes per frame | Count sign changes / frame_length | Distinguish noise vs tonal content |
| **Temporal Centroid** | Center of mass in time | Weighted mean of time indices by energy | Attack/decay character |
| **Log Attack Time** | Time from onset to peak | `log10(t_peak - t_onset)` | Percussive vs sustained matching |

#### Spectral Descriptors

| Descriptor | Description | Computation | Use in Concatenative Synthesis |
|---|---|---|---|
| **Spectral Centroid** | Center of mass of the spectrum | `sum(f * |X(f)|) / sum(|X(f)|)` | Brightness/timbre matching; correlates to perceived brightness |
| **Spectral Spread** | Variance around the centroid | `sqrt(sum((f - centroid)^2 * |X(f)|) / sum(|X(f)|))` | Bandwidth character |
| **Spectral Flatness** | Ratio of geometric to arithmetic mean of spectrum | `geomean(|X(f)|) / mean(|X(f)|)` | Noisiness vs tonality (0=tonal, 1=noise) |
| **Spectral Rolloff** | Frequency below which N% of energy is concentrated | Frequency at 85th/95th percentile of cumulative energy | High-frequency content |
| **Spectral Flux** | Frame-to-frame spectral change | `sum((|X_t(f)| - |X_{t-1}(f)|)^2)` | Onset detection, temporal evolution |
| **Spectral Tilt / Slope** | Linear regression slope of the log spectrum | First-order autocorrelation of spectrum | Timbral character |
| **Inharmonicity** | Deviation of partials from harmonic series | Weighted divergence from integer multiples of f0 | Distinguish harmonic from inharmonic |

#### Mel-Frequency Cepstral Coefficients (MFCCs)

MFCCs are the single most important descriptor family for concatenative synthesis. They provide a compact representation of the spectral envelope that approximates human auditory perception.

**MFCC Computation Pipeline:**

```
1. Pre-emphasis:       y[n] = x[n] - alpha * x[n-1]     (alpha ~ 0.97)
2. Framing:            Split into overlapping frames (20-40ms, 50% overlap)
3. Windowing:          Apply Hamming/Hann window to each frame
4. FFT:                Compute magnitude spectrum |X(k)|
5. Mel Filterbank:     Apply 20-40 triangular filters spaced on mel scale
                       mel(f) = 2595 * log10(1 + f/700)
6. Log compression:    Take log of each filter's energy sum
7. DCT:                Apply Discrete Cosine Transform
8. Truncate:           Keep first 13-20 coefficients
```

**Why MFCCs work for concatenative synthesis:**
- They capture the spectral **envelope** (formant structure) while discarding fine harmonic detail
- Mel spacing models human frequency perception (logarithmic at high frequencies)
- Log compression models human loudness perception
- DCT decorrelates the features, concentrating information in few coefficients
- Typical usage: 13 MFCCs + delta (first derivative) + delta-delta (second derivative) = 39 features

#### Pitch Descriptors

| Descriptor | Description | Common Algorithm |
|---|---|---|
| **Fundamental Frequency (f0)** | Perceived pitch | YIN algorithm, pYIN, autocorrelation |
| **Pitch Confidence** | Reliability of f0 estimate | Aperiodicity measure from YIN |
| **Pitch Chroma** | Pitch class (0-11) regardless of octave | `f0 mod 12` on MIDI scale |
| **Voicing / Periodicity** | Degree of harmonic structure | HNR (harmonics-to-noise ratio) |

#### High-Level / Semantic Descriptors

| Descriptor | Description |
|---|---|
| **Instrument Class** | Categorical label (strings, brass, voice, etc.) |
| **Onset Strength** | Magnitude of detected onset |
| **Beat Position** | Phase relative to detected beat grid |
| **Phoneme Label** | For speech corpora |

### Descriptor Weighting

In practice, descriptors are weighted according to their perceptual importance and the specific use case:

```cpp
float distance = 0.0f;
distance += w_mfcc     * euclidean(target.mfcc, unit.mfcc);
distance += w_pitch    * sqr(target.pitch - unit.pitch);
distance += w_energy   * sqr(target.energy - unit.energy);
distance += w_centroid * sqr(target.centroid - unit.centroid);
distance += w_zcr      * sqr(target.zcr - unit.zcr);
// etc.
```

Common default weightings prioritize MFCCs and pitch, with lower weights on energy and spectral descriptors.

---

## 5. Real-Time vs Offline

### Can Concatenative Synthesis Run in Real-Time?

**Yes**, but with significant architectural constraints. Multiple systems have demonstrated real-time performance:

- **CataRT** (2005+): Real-time in Max/MSP
- **C-C-Combine** (2012+): Real-time in Max/MSP, analyzing every 10ms
- **The Concatenator** (2024): Real-time VST plugin
- **Catecophony**: Real-time VST3/AU plugin (experimental)

### Real-Time Constraints and Solutions

#### Latency Budget
At 44.1kHz with a 512-sample buffer: **~11.6ms** per audio callback. All analysis, selection, and synthesis must complete within this window.

| Operation | Typical Cost | Strategy |
|---|---|---|
| FFT (1024-point) | ~10-50 us | Acceptable per callback |
| MFCC extraction | ~50-200 us | Acceptable per callback |
| KD-tree lookup | ~1-10 us | Near-instantaneous |
| Full Viterbi (large corpus) | ~1-100 ms | TOO SLOW for real-time; use greedy or particle filter |
| Crossfade/OLA | ~5-20 us | Trivial |

#### Architecture for Real-Time

```
Audio Thread (real-time, lock-free):
  |-- Read input buffer
  |-- Compute FFT of current frame
  |-- Extract descriptors (pre-computed filterbanks)
  |-- Query KD-tree for nearest neighbor(s)
  |-- Read selected unit from corpus buffer
  |-- Apply crossfade envelope
  |-- Write to output buffer

Background Thread (non-real-time):
  |-- Corpus loading and segmentation
  |-- Full descriptor extraction for corpus
  |-- KD-tree construction
  |-- Optional: pre-compute descriptor statistics
```

#### Key Real-Time Design Decisions

1. **Pre-compute everything possible**: Corpus analysis, KD-tree construction, and descriptor normalization happen at load time, not in the audio callback.

2. **Greedy selection over Viterbi**: Use nearest-neighbor lookup (O(log n) with KD-tree) instead of full Viterbi search (O(n * m^2)). Sacrifice global optimality for latency.

3. **Fixed analysis hop size**: Analyze target audio at regular intervals (e.g., every 512 or 1024 samples) to align with buffer callbacks.

4. **Lock-free corpus access**: The audio thread reads corpus audio buffers and KD-tree indices without locks. Corpus modifications (loading new audio) happen on a background thread with atomic pointer swaps.

5. **Limit descriptor dimensionality**: Fewer descriptors = faster distance computation and KD-tree queries. 5-13 descriptors is typical for real-time.

6. **Particle filter approach**: The Concatenator (2024) demonstrated that particle filters achieve computational complexity **independent of corpus size**, depending only on particle count. This is ideal for very large corpora.

### Offline Advantages

Offline systems like **AudioGuide** can:
- Use full Viterbi search for globally optimal unit sequences
- Layer units much more densely (hundreds of simultaneous voices)
- Perform multiple search passes with different descriptor weightings
- Apply expensive post-processing (spectral smoothing, PSOLA)
- Handle corpora of unlimited size without performance constraints

---

## 6. Notable Implementations

### Academic / Open Source

#### CataRT (IRCAM, 2005-present)
- **Developer**: Diemo Schwarz et al., IRCAM
- **Platform**: Max/MSP with FTM&Co or MuBu extensions; standalone app
- **Architecture**: Real-time corpus-based concatenative synthesis
- **Features**: ~230 MPEG-7 descriptors, KD-tree (via mubu.knn), interactive 2D descriptor space navigation, multiple segmentation strategies (onset, silence, fixed), YIN pitch tracking
- **Selection**: Weighted Euclidean distance, minimum-distance or random within radius
- **Synthesis**: Configurable fade envelopes, pitch via resampling, loudness adjustment
- **License**: GPL (free/libre open source)
- **URL**: http://imtr.ircam.fr/imtr/CataRT

#### AudioGuide (Ben Hackbarth et al.)
- **Platform**: Python + Csound (offline rendering)
- **Architecture**: Offline concatenative synthesis with hierarchical multi-pass search
- **Key Feature**: Non-real-time allows much denser layering and more thorough search
- **Workflow**: User writes Python config files specifying target, corpus, descriptor weights, and constraints; program outputs soundfile event lists
- **Outputs**: Csound synthesis, Max/MSP events, Logic/Pro Tools sessions, musical notation (bach.roll)
- **URL**: https://github.com/benhackbarth/audioguide

#### C-C-Combine (Rodrigo Constanzo, 2012+)
- **Platform**: Max/MSP with Alex Harker's externals
- **Architecture**: Real-time, analyzes every 10ms
- **Descriptors**: Loudness, pitch, spectral centroid, spectral flatness (mean, min, max)
- **Design**: Built for accessibility -- no large framework dependencies, simple to use
- **URL**: https://rodrigoconstanzo.com/combine/

#### Catecophony (Ben Hayes)
- **Platform**: C++/JUCE (VST3/AU)
- **Architecture**: Real-time corpus-based concatenative synthesis
- **Libraries**: Essentia (feature extraction), FFTW3 (FFT)
- **Status**: Experimental/alpha
- **URL**: https://github.com/ben-hayes/catecophony

### Commercial Products

#### AudioTexture (Le Sound / AudioGaming)
- **Format**: VST, VST3, AU, AAX
- **Architecture**: Concatenative synthesis with adaptive (non-equal) unit sizes
- **Descriptors exposed to user**: Energy, Noisiness (high-frequency content), Brightness (spectral centroid)
- **Key Feature**: Infinite sound texture generation from a single source sample
- **Use Case**: Sound design, film/game audio, ambient texture creation
- **Also available**: AudioTexture Free (with Freesound integration)
- **URL**: https://www.kvraudio.com/product/audiotexture-by-le-sound

#### The Concatenator (DataMind Audio, 2024-2025)
- **Format**: VST3, AU, AAX
- **Architecture**: Bayesian particle filter for real-time audio mosaicing
- **Key Innovation**: Computational complexity independent of corpus size
- **Parameters**: Particle Count + Polyphony, Variation (sample repetition control), Particle Reset (re-scatter probabilities), Stickiness (hold probability)
- **Based on**: ISMIR 2024 paper by Chris Tralie and Ben Cantil
- **URL**: https://datamindaudio.ai/product/concatenator/

---

## 7. Relationship to Corpus-Based Synthesis

### Terminology Hierarchy

```
Sound Synthesis
  |
  +-- Concatenative Synthesis (broad category)
       |
       +-- Diphone Synthesis (fixed small database, early speech synthesis)
       |
       +-- Unit Selection Synthesis (large database, cost-function-based selection)
       |
       +-- Corpus-Based Concatenative Synthesis (CBCS)
            |   (large database + descriptor analysis + content-based selection)
            |
            +-- Audio Mosaicing / Musaicing
            |   (reconstruct a target audio using corpus fragments)
            |
            +-- Interactive CBCS
                (real-time navigation of descriptor space)
```

### Key Distinctions

- **Concatenative synthesis** is the broadest term: any technique that joins pre-recorded audio units.
- **Corpus-based concatenative synthesis (CBCS)** is a specific refinement that emphasizes:
  1. Large, diverse audio databases (not just diphones or small fixed sets)
  2. Rich descriptor analysis of all units
  3. Content-based selection using MIR techniques
  4. Dynamic, flexible unit boundaries
- **Audio mosaicing** (or **musaicing**) specifically refers to the use case where a target audio signal is reconstructed from corpus fragments -- the audio equivalent of a photo mosaic.

In modern usage (post-2006), "concatenative synthesis" in the music/audio context almost always refers to corpus-based concatenative synthesis. The terms are used nearly interchangeably in the music technology community, though "corpus-based" is more precise.

---

## 8. C++/JUCE Implementation Considerations

### Architecture Overview

```
+----------------------------------------------------------+
|                    JUCE AudioProcessor                     |
+----------------------------------------------------------+
|                                                            |
|  +-----------------+    +-----------------------------+   |
|  | Corpus Manager  |    | Real-Time Analysis Engine   |   |
|  |                 |    |                             |   |
|  | - AudioBuffer   |    | - FFT (juce::dsp::FFT)     |   |
|  | - Unit metadata |    | - MFCC extraction          |   |
|  | - KD-Tree index |    | - Spectral descriptors     |   |
|  | - Segmentation  |    | - Pitch detection (YIN)    |   |
|  +-----------------+    +-----------------------------+   |
|          |                          |                      |
|          v                          v                      |
|  +--------------------------------------------------+    |
|  |            Unit Selection Engine                   |    |
|  |                                                    |    |
|  | - KD-tree nearest neighbor query                   |    |
|  | - Weighted Euclidean distance                      |    |
|  | - Optional concatenation cost                      |    |
|  +--------------------------------------------------+    |
|                          |                                 |
|                          v                                 |
|  +--------------------------------------------------+    |
|  |            Synthesis / Output Engine                |    |
|  |                                                    |    |
|  | - Unit playback from corpus buffer                 |    |
|  | - Crossfade / overlap-add                          |    |
|  | - Gain normalization                               |    |
|  +--------------------------------------------------+    |
+----------------------------------------------------------+
```

### Feature Extraction Libraries for C++

| Library | Language | Real-Time | Key Features | Integration |
|---|---|---|---|---|
| **Essentia** | C++ (with Python bindings) | Yes | 200+ descriptors, MFCC, pitch, spectral | Used by Catecophony; GPL license |
| **Gist** | C++ | Yes | Lightweight, core descriptors (MFCC, spectral centroid, etc.) | Header-only, easy to integrate |
| **Aubio** | C | Yes | Onset detection, pitch detection, tempo | C API, easy to wrap |
| **JUCE DSP** | C++ | Yes | FFT, windowing, filters | Already in JUCE; no MFCC built-in |
| **Custom** | C++ | Yes | Full control, minimal dependencies | Most work but most flexibility |

### Implementing MFCCs in JUCE (Pseudocode)

```cpp
class MFCCExtractor {
    static constexpr int fftSize = 2048;
    static constexpr int numMelFilters = 40;
    static constexpr int numCoefficients = 13;

    juce::dsp::FFT fft { (int)std::log2(fftSize) };
    std::array<std::array<float, fftSize/2+1>, numMelFilters> melFilterbank;

    void prepareMelFilterbank(float sampleRate) {
        float melMin = hzToMel(0.0f);
        float melMax = hzToMel(sampleRate / 2.0f);
        // Create numMelFilters+2 equally spaced points on mel scale
        // Build triangular filters in linear frequency domain
        // Store in melFilterbank matrix
    }

    std::array<float, numCoefficients> extract(const float* frame) {
        // 1. Apply window (Hamming)
        // 2. Compute FFT -> magnitude spectrum
        // 3. Apply mel filterbank (matrix multiply)
        // 4. Log compress: log(max(filterEnergies, 1e-10))
        // 5. DCT of log energies
        // 6. Return first numCoefficients values
    }

    static float hzToMel(float hz) {
        return 2595.0f * std::log10(1.0f + hz / 700.0f);
    }
    static float melToHz(float mel) {
        return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
    }
};
```

### KD-Tree Implementation Notes

For real-time concatenative synthesis, a KD-tree is essential for efficient nearest-neighbor search in descriptor space.

Options:
1. **nanoflann** (header-only C++ KD-tree library, BSD license) -- highly recommended for JUCE integration
2. **FLANN** (Fast Library for Approximate and Exact Nearest Neighbors)
3. **Custom implementation** -- straightforward for low-dimensional spaces (5-13 dimensions)

Key considerations:
- Build the tree on the background thread when corpus is loaded
- Query the tree from the audio thread (read-only, thread-safe)
- For dimensions > ~20, KD-trees degrade toward brute-force; keep descriptor count manageable
- Use approximate nearest neighbor for even faster queries if needed

### Audio Thread Safety

```cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) {
    // Audio thread -- NO allocations, NO locks, NO blocking

    for (int frame = 0; frame < buffer.getNumSamples(); frame += hopSize) {
        // 1. Extract descriptors from input (or use pre-analyzed target)
        auto descriptors = analyzer.extract(buffer, frame);

        // 2. Query KD-tree (read-only, thread-safe)
        auto nearestUnit = kdTree.nearestNeighbor(descriptors);

        // 3. Read unit audio from pre-loaded corpus buffer
        //    (lock-free access to shared AudioBuffer)

        // 4. Apply crossfade and write to output
        synthesizer.outputUnit(nearestUnit, buffer, frame);
    }
}
```

### Practical Descriptor Set for Real-Time VST

For a practical real-time implementation, start with these 8-10 descriptors:

1. **MFCCs (13 coefficients)** -- spectral envelope / timbre
2. **Spectral Centroid** -- brightness
3. **Spectral Flatness** -- noisiness vs tonality
4. **RMS Energy** -- loudness
5. **Zero-Crossing Rate** -- noise/transient character
6. **Fundamental Frequency (f0)** -- pitch (via YIN or autocorrelation)
7. **Spectral Flux** -- rate of spectral change

This gives a descriptor vector of ~18-19 dimensions (13 MFCCs + 6 scalar descriptors), which is well within KD-tree efficiency range.

---

## Sources

- [Concatenative Synthesis - Wikipedia](https://en.wikipedia.org/wiki/Concatenative_synthesis)
- [Schwarz, D. (2006) "Concatenative Sound Synthesis: The Early Years"](https://hal.science/hal-01161361)
- [Schwarz, D. et al. (2006) "Real-Time Corpus-Based Concatenative Synthesis with CataRT"](https://www.researchgate.net/publication/224927716_Real-Time_Corpus-Based_Concatenative_Synthesis_with_CataRT)
- [Schwarz, D. "Interacting with a Corpus of Sounds"](https://econtact.ca/16_2/schwarz_corpus.html)
- [Schwarz, D. "Principles and Applications of Interactive Corpus-Based Concatenative Synthesis"](http://jim.afim-asso.org/jim08/upload/05_Schwarz_catart-jim2008-final.pdf)
- [Hunt, A. & Black, A. (1996) "Unit Selection in a Concatenative Speech Synthesis System"](https://www.ee.columbia.edu/~dpwe/e6820/papers/HuntB96-speechsynth.pdf)
- [Tralie, C. & Cantil, B. (2024) "The Concatenator: A Bayesian Approach to Real Time Concatenative Musaicing"](https://arxiv.org/abs/2411.04366)
- [Aalto University - Concatenative Speech Synthesis](https://speechprocessingbook.aalto.fi/Synthesis/Concatenative_speech_synthesis.html)
- [AudioGuide - Ben Hackbarth](https://github.com/benhackbarth/audioguide)
- [C-C-Combine - Rodrigo Constanzo](https://rodrigoconstanzo.com/combine/)
- [Catecophony - Ben Hayes](https://github.com/ben-hayes/catecophony)
- [AudioTexture - Le Sound / KVR](https://www.kvraudio.com/product/audiotexture-by-le-sound)
- [The Concatenator - DataMind Audio](https://datamindaudio.ai/product/concatenator/)
- [CataRT - IRCAM](http://imtr.ircam.fr/imtr/CataRT)
- [Corpus Based Synthesis - IRCAM](http://imtr.ircam.fr/imtr/Corpus_Based_Synthesis)
- [Gist C++ Audio Analysis Library](https://github.com/adamstark/Gist)
- [Essentia C++ Audio Analysis Library](https://github.com/MTG/essentia)
- [Schwarz, D. - Google Scholar](https://scholar.google.co.uk/citations?user=uVVKDWAAAAAJ&hl=en)
