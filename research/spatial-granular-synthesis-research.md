---
title: "Spatial Granular Synthesis: Academic Research Compilation"
created: 2026-02-08
domain: spatial-audio
type: research
keywords:
  - spatial-granular
  - academic-research
  - ambisonics
  - granular-synthesis
  - spatialization
---
# Spatial Granular Synthesis: Academic Research Compilation

**Researched:** 2026-02-07
**Domain:** Per-grain spatial processing, HRTF convolution, Ambisonic granular synthesis, spatial perception
**Target:** JUCE C++ audio plugin development (O-GrainScatter spatial features)
**Confidence:** HIGH (academic sources, open-source implementations) / MEDIUM (perceptual thresholds from single studies)

---

## Table of Contents

1. [Granular Spatialisation as Sound Diffusion (ICMC 2016)](#1-granular-spatialisation-as-sound-diffusion)
2. [Spatial Granular Synthesis with Ambitools (IFC 2024)](#2-spatial-granular-synthesis-with-ambitools)
3. [EmissionControl2: Per-Grain Processing Architecture (CMJ 2021)](#3-emissioncontrol2-per-grain-processing-architecture)
4. [Perceptual Evaluation of Listener Envelopment (JAES 2023)](#4-perceptual-evaluation-of-listener-envelopment)
5. [Perception of Granular Sounds in High-Order Ambisonics (OPUS 2020)](#5-perception-of-granular-sounds-in-high-order-ambisonics)
6. [IEM GranularEncoder: Production-Ready Ambisonic Granular Plugin](#6-iem-granularencoder)
7. [Spatial Granular Synthesis in SuperCollider (HOA)](#7-spatial-granular-synthesis-in-supercollider)
8. [Spectral and Granular Spatialization with Boids (ICMC 2006)](#8-spectral-and-granular-spatialization-with-boids)
9. [Spectral and 3D Spatial Granular Synthesis in Csound (ICSC 2017)](#9-spectral-and-3d-spatial-granular-synthesis-in-csound)
10. [Efficient HRTF Convolution for Binaural Rendering](#10-efficient-hrtf-convolution-for-binaural-rendering)
11. [3D Tune-In Toolkit: Per-Source Binaural Architecture](#11-3d-tune-in-toolkit)
12. [HRTF Preprocessing for Ambisonics Rendering](#12-hrtf-preprocessing-for-ambisonics-rendering)
13. [Steam Audio: Production Binaural Pipeline](#13-steam-audio-production-binaural-pipeline)
14. [NIME 2025: Towards Per-Grain Parameterisation](#14-nime-2025-per-grain-parameterisation)
15. [Implementation Strategy Synthesis](#15-implementation-strategy-synthesis)

---

## 1. Granular Spatialisation as Sound Diffusion

**Citation:** "Granular Spatialisation, a new method for sound diffusion in high-density arrays of speakers." Proceedings of the International Computer Music Conference (ICMC), 2016.
**Source:** [University of Michigan Digital Library](https://quod.lib.umich.edu/cgi/p/pod/dod-idx/granular-spatialisation-a-new-method-for-sound-diffusion.pdf?c=icmc&idno=bbp2372.2016.006&format=pdf)
**Also:** [ResearchGate](https://www.researchgate.net/publication/318028798)

### Core Concept

Granular Spatialisation treats spatial diffusion itself as a granular process. Rather than spatializing a pre-existing granular stream, the method applies granular decomposition to the diffusion path -- each grain is independently routed to a unique speaker or speaker group in a high-density array.

### Technical Implementation

- **Speaker array:** 124-138 speakers in an immersive configuration (Virginia Tech Spatial Audio Workshops, August 2015)
- **Platform:** Max/MSP patchers for real-time control
- **Diffusion method:** Continuous linear diffusion across the full speaker array
- **Grain routing:** Each grain assigned to individual speakers or speaker subsets, creating spatially distributed textures from monophonic source material

### Key Algorithmic Principles

1. **Spatial grain scheduling:** Grains are not merely panned but *assigned* to discrete speaker locations, treating each speaker as a spatial "slot"
2. **Time-dynamic spatialisation:** Spatial parameters evolve over time, allowing grain clouds to migrate, expand, contract across the array
3. **Adjustable spatial density:** The ratio of active speakers to total speakers at any moment controls spatial density independently from temporal grain density

### Significance for Plugin Development

This work demonstrates that spatial distribution can be treated as an independent granular parameter. The key insight is decoupling spatial density from temporal density -- you can have temporally sparse but spatially dense grain clouds, or vice versa.

---

## 2. Spatial Granular Synthesis with Ambitools

**Citation:** Lecomte, P. & Fernandez, J.M. "Spatial Granular Synthesis With Ambitools and AntesCollider." Proceedings of the 4th International Faust Conference (IFC), November 2024, Turin, Italy.
**Source:** [HAL Archives](https://hal.science/hal-04846653v1) | [Faust Conference Proceedings](https://faust.grame.fr/community/ifc/2024/Spatial_Granular_Synthesis_With_Ambitools_And_Supercollider.pdf)

### Architecture Overview

The system implements a spatial granular synthesis tool in the Faust language as part of the Ambitools v1.3 library ("Granulator" plugin). It generates **N parallel monophonic grain streams**, each independently spatialized using Higher Order Ambisonics (HOA).

### Signal Flow

```
Source Audio (file or live circular buffer)
    |
    v
N parallel grain generators (each produces mono grain stream)
    |
    v
Per-stream HOA Encoder: each mono stream -> (L+1)^2 HOA channels
    |
    v
HOA Bus Summation (all N streams summed in HOA domain)
    |
    v
HOA Decoder (loudspeaker array or binaural)
```

### HOA Encoding Formula

Each monophonic grain stream `i` is encoded into HOA signals of order L:

```
Number of HOA channels = (L + 1)^2

For L=1 (1st order): 4 channels
For L=3 (3rd order): 16 channels
For L=5 (5th order): 36 channels
For L=7 (7th order): 64 channels
```

### Spherical Shell Sector Geometry

Grains are distributed within a **spherical shell sector** defined by:
- **Azimuth range:** [azim_min, azim_max]
- **Elevation range:** [elev_min, elev_max]
- **Radius range:** [r_min, r_max] (for near/far-field distribution)

The swarm geometry provides controllable spatial spread without requiring per-grain manual positioning.

### Dual Input Modes

1. **File mode:** Grains seeded from pre-recorded sound files with random or sequential position selection
2. **Live mode:** Circular buffer captures incoming audio; grains seeded from buffer with configurable look-back time

### AntesCollider Integration

The Faust DSP is wrapped for use in AntesCollider (Antescofo + SuperCollider), providing:
- Score-driven temporal control over grain swarm evolution
- Real-time visualization of the grain swarm spatial distribution
- Programmatic control of all synthesis and spatial parameters

### Key Implementation Detail

The Faust implementation compiles to efficient C++ code, making this architecture directly portable to JUCE plugin development. The per-stream (rather than per-grain) HOA encoding is a practical optimization -- each of the N parallel streams handles its own grain scheduling and spatial position, avoiding the cost of encoding/decoding thousands of individual grains per second.

---

## 3. EmissionControl2: Per-Grain Processing Architecture

**Citation:** Roads, C., Kilgore, J., & DuPlessis, R. "Architecture for Real-Time Granular Synthesis With Per-Grain Processing: EmissionControl2." Computer Music Journal, Vol. 45, No. 3, pp. 20-40, Fall 2021.
**Source:** [MIT Press](https://direct.mit.edu/comj/article-abstract/45/3/20/113899/) | [Project MUSE](https://muse.jhu.edu/article/882422/summary) | [GitHub](https://github.com/EmissionControl2/EmissionControl2)

### Per-Grain Signal Processing Chain

EmissionControl2 implements true per-grain processing, where each grain is rendered as a unique particle of sound. The per-grain signal chain is:

```
Grain Source (waveform selection / file position)
    |
    v
Grain Envelope (per-grain shape)
    |
    v
Amplitude (per-grain level)
    |
    v
Frequency / Pitch (per-grain transposition)
    |
    v
Filter (per-grain center frequency + resonance)
    |
    v
Spatial Position (per-grain panning)
    |
    v
Output mix
```

### Per-Grain Parameters

Each individual grain receives independent values for:
1. **Envelope shape** -- per-grain window function selection
2. **Waveform** -- source material or synthesis waveform per grain
3. **Amplitude** -- independent level control
4. **Frequency** -- per-grain pitch/transposition
5. **Spatial position** -- independent panning per grain
6. **Filter center frequency** -- per-grain spectral shaping
7. **Filter resonance** -- per-grain filter Q

### Grain Emission Modes

- **Synchronous:** Grains emitted at regular intervals (deterministic streams)
- **Asynchronous:** Grains emitted at stochastic intervals (clouds, textures)
- **Intermittency control:** Probability-based grain suppression for sparse textures

### Modulation System

- **6 LFOs** with bipolar or unipolar waveforms
- Each LFO routable to any synthesis parameter
- LFO shapes: sine, triangle, square, saw, random (sample-and-hold), noise
- Enables gestural design through parameter modulation

### Capacity and Performance

- **Up to 2048 simultaneous grains** -- indicates the ceiling for real-time per-grain processing on modern hardware
- Multi-source granulation (multiple sound files simultaneously)
- MIDI/OSC mapping for all parameters
- Algorithmic control via OSC scripting

### Filter Design

The paper describes a "unique filter design optimized for per-grain synthesis" -- this implies filter state management that handles the discontinuous nature of grains (each grain gets its own filter instance rather than sharing state across the grain stream).

### Architectural Significance

EC2's architecture proves that per-grain spatial positioning is computationally feasible for 2048+ simultaneous grains in real-time. The key optimization is that per-grain "spatial position" uses amplitude-based panning (stereo or multichannel VBAP) rather than per-grain HRTF convolution, keeping the spatial cost to simple gain calculations per grain.

---

## 4. Perceptual Evaluation of Listener Envelopment

**Citation:** Riedel, S., Frank, M., & Zotter, F. "The Effect of Temporal and Directional Density on Listener Envelopment." Journal of the Audio Engineering Society, Vol. 71, No. 7/8, pp. 455-467, July/August 2023.
**Preprint:** [arXiv:2301.10210](https://arxiv.org/abs/2301.10210)
**Code/Data:** [GitHub - stefanriedel/JAES_SpatialGranularSynthesis](https://github.com/stefanriedel/JAES_SpatialGranularSynthesis)

### Core Research Question

What temporal and directional conditions are needed to create the sensation of being enveloped by sound? Is maximum envelopment achieved by a stationary isotropic diffuse field, or by a field with audible spatio-temporal fluctuations?

### Spatial Granular Synthesis as Experimental Tool

The researchers used spatial granular synthesis as a precision instrument to independently control:
- **Temporal density:** Time interval between grain onsets (Delta-t)
- **Directional density:** Number and distribution of unique directions from which grains arrive

### Grain Scheduling Algorithm

The system functions as a **time-varying multichannel FIR filter**:

```
Input: single-channel buffer x(t)
Parameters:
  - L: grain length (seconds)
  - Q: query/seed region (seconds) -- limits random seed position range
  - Delta-t: inter-grain onset interval (seconds)
  - Spatial distribution: assignment of grains to loudspeaker directions

Each grain is windowed (e.g., Hann window), creating time-varying filter taps
that fade in/out, producing the grain envelope.
```

### Critical Perceptual Thresholds

**THE KEY FINDING -- Temporal Fusion Threshold:**

> A directionally uniform distribution of sound events at time intervals **Delta-t < 20 milliseconds** is required to elicit a sensation of diffuse envelopment. Longer time intervals lead to localized auditory events.

This 20ms threshold is critical for plugin implementation:
- **Delta-t < 20ms** = grains fuse into continuous spatial texture (diffuse envelopment)
- **Delta-t > 20ms** = individual grains perceived as discrete localized events
- At 48kHz, 20ms = 960 samples between grain onsets

### Envelopment vs. Engulfment

Two distinct perceptual attributes were measured:

1. **Listener Envelopment (LEV):** Sensation of being surrounded by sound in the horizontal plane
2. **Listener Engulfment:** Sensation of being covered by sound from above (proposed by Sazdov et al.)

### Height Layer Findings

- **Elevated loudspeaker layers do NOT increase envelopment** (horizontal sensation)
- **Elevated layers DO contribute specifically to engulfment** (overhead coverage sensation)
- This means height channels are perceptually distinct from surround channels for granular textures

### Spectral Filtering Effect

- **Lowpass-filtered stimuli increase envelopment** perception
- But lowpass filtering **decreases control over engulfment**
- Implication: High-frequency content aids the separation of envelopment from engulfment

### Experimental Setup

- Three-layer height loudspeaker system
- Directionally uniform grain distributions tested
- Multiple temporal densities from sub-20ms to clearly separated events
- Auditory cues measured: mean interaural coherence, ITD standard deviation, ILD standard deviation, monaural spectral differences

### Implementation Implications

For a granular spatial plugin:
- **Grain density >= 50 grains/sec** (20ms intervals) creates continuous spatial textures
- Below this threshold, listeners hear discrete spatial events (which can be musically useful)
- Height processing is worth implementing but serves a different perceptual function than surround
- Lowpass filtering of the grain cloud enhances the sense of immersion

---

## 5. Perception of Granular Sounds in High-Order Ambisonics

**Citation:** Rossetti, D. & Manzolli, J. "Studying the Perception of Sound in Space: Granular Sounds Spatialized in a High-Order Ambisonics System." OPUS, Vol. 26, No. 2, pp. 1-26, May/August 2020.
**Source:** [OPUS Journal](https://www.anppom.com.br/revista/index.php/opus/article/view/opus2020b2610)

### Experimental Setup

- **Location:** CIRRMT's Immersive Presence Laboratory, McGill University
- **System:** High-Order Ambisonics loudspeaker array
- **Analysis method:** Graphical representations derived from audio descriptors (spectral centroid curves, loudness curves, volume representations, phase space graphics)

### Research Hypotheses Tested

1. **H1:** Variation of granular synthesis parameters produces differences in the morphology of sounds perceived in their time-varying spatial distribution
2. **H2:** Higher orders of ambisonics produce more morphological sound variations, and generate sound fields with more depth

### Key Findings

#### Ambisonic Order Effects on Granular Perception

- **Higher Ambisonic orders** cause listeners to perceive **more variations in frequency and intensity** during spatial granular playback
- **Decorrelation effect:** Higher orders produce a **preponderance of decorrelation**, creating more diffuse sound fields
- The decorrelation effect is directly related to increased spatial resolution of the Ambisonic encoding

#### Grain Parameter-Spatial Perception Interaction

- Granular synthesis parameters (grain size, density, pitch, rarefaction rate) interact with the Ambisonic order to produce different spatial morphologies
- The **timbral quality** of grains is perceived differently when spatialized at different Ambisonic orders -- the same grain parameters produce different perceived timbral results depending on spatial resolution
- Spatial distribution of grains creates "three-dimensional spatial morphology" when grains scatter to unique locations

#### Audio Descriptors Used

- **Spectral centroid:** Tracks brightness variations across spatial distribution
- **Loudness curves:** Measures perceived level fluctuations
- **Phase space graphics:** Visualizes the dynamic evolution of spectral-spatial interactions

### Implications for Plugin Development

- Ambisonic order directly affects the perceived timbral character of granular textures, not just spatial accuracy
- Even 1st-order Ambisonics produces meaningful spatial granular effects, but higher orders (3rd+) significantly enhance the richness of spatial textures
- Per-grain spatial encoding at higher Ambisonic orders creates a "decorrelation" effect that is perceptually desirable for immersive textures

---

## 6. IEM GranularEncoder

**Citation:** IEM Plug-in Suite, Institute of Electronic Music and Acoustics, University of Music and Performing Arts Graz.
**Source:** [IEM Plugin Documentation](https://plugins.iem.at/docs/granularencoder/)

### Core Functionality

The GranularEncoder is an **Ambisonic granular synthesis plug-in where each grain is encoded to an individual direction.** It processes mono or stereo input, taking brief audio segments and placing them independently in the spatial domain.

### Grain Parameters

| Parameter | Range | Notes |
|-----------|-------|-------|
| **Length** | 1-200 ms | Grains < 50ms lack full frequency spectrum fidelity |
| **Delta-t** | configurable | Time interval between grain generation |
| **Position** | 0.0 - buffer length | Index into circular stereo buffer (0.0 = present, >0.0 = pre-delay) |

### Spatial Distribution Methods

Two geometric distribution modes:

1. **3D Spherical Cap:**
   - Controllable spread in both azimuth AND elevation
   - Defined by the **Size** parameter (opening angle of the cap)
   - Grains distributed randomly within the spherical cap region

2. **2D Circular:**
   - Controllable spread in azimuth only
   - Allows precise grain spreading on a specific elevation level
   - Better for horizontal-only spatial effects

### Shape Control

The **Shape** parameter controls the probability distribution within the spatial region:
- **Negative values:** Edge-weighted distribution (grains cluster at boundary)
- **Zero:** Uniform distribution (equal probability everywhere)
- **Positive values:** Center-peaked distribution (grains cluster at center)

This is a key design element -- it provides perceptual control over spatial density without changing grain density.

### Additional Controls

- **Source:** Select grain seeding from left (-1), right (1), or both (0) channels of stereo input
- **Mix:** Blend between dry (standard Ambisonic encoding of the full signal) and wet (granular encoding)
- **Freeze mode:** Halts buffer input, allows bidirectional pitch shifting

### Operating Constraints

- **Real-time mode:** Pitch limited to downshifts only (causality constraint -- cannot look into the future)
- **Freeze mode:** Full bidirectional pitch shifting possible

### Implementation Significance

The IEM GranularEncoder is a **production-ready reference implementation** of per-grain Ambisonic encoding. Its parameter set (Length, Delta-t, Position, Size, Shape, Source, Mix) provides a validated UI/UX model for spatial granular controls.

---

## 7. Spatial Granular Synthesis in SuperCollider

**Source:** [RingBuffer - Spatial Granular in SuperCollider](http://ringbuffer.org/spatial_audio/spatial_synthesis/spatial_granular_supercollider/)

### Architecture: Parallel Grain Streams with Per-Stream HOA Encoding

```
16 independent TGrains UGens (parallel grain synths)
    |
    v (each stream independently)
HOAEncoder.ar(3, grain_signal, azimuth, elevation)
    |
    v
16-channel HOA bus (3rd order = (3+1)^2 = 16 channels)
    |
    v
HOABinaural.ar(3, hoa_bus) -- binaural decode for headphones
    |
    v
Stereo output
```

### Per-Stream Spatial Parameters

Each of the 16 grain streams gets independent spatial encoding:

```supercollider
// Per-stream azimuth assignment (0.1 radian increments)
Synth(\encoder, [inbus, ~grain_BUS.index + i, azim, i * 0.1], ~encoder_GROUP)
```

### Grain Generation

Using the `TGrains` UGen:
- Controllable rate (pitch transposition)
- Buffer position mapping: `c = pos * BufDur.kr(buffer)` (normalized 0-1 position to sample position)
- Grain duration: 0.1 seconds (100ms) in the reference implementation
- Trigger-based grain emission

### HOA Specifics

- **Order:** 3rd order Ambisonics
- **Channels:** 16 ((3+1)^2)
- **Encoding:** `HOAEncoder.ar(3, signal, azimuth, elevation)` per stream
- **Decoding:** `HOABinaural.ar(3, ...)` for headphone output
- **HRIR data:** Pre-loaded via `HOABinaural.loadbinauralIRs(s)`

### Performance Architecture

Signal flow is segregated into distinct synth groups with explicit ordering:
1. **Grain generation group** (addAction: 'addToHead')
2. **Encoder group** (addAction: 'addAfter' grain group)
3. **Decoder group** (addAction: 'addAfter' encoder group)

This group-based ordering is analogous to JUCE's `AudioProcessorGraph` node ordering.

---

## 8. Spectral and Granular Spatialization with Boids

**Citation:** Kim-Boyle, D. "Spectral and Granular Spatialization with Boids." Proceedings of the International Computer Music Conference (ICMC), New Orleans, pp. 139-142, 2006.
**Source:** [Semantic Scholar](https://www.semanticscholar.org/paper/Spectral-and-Granular-Spatialization-with-Boids-Kim-Boyle/687c5a827614027c4a9199708a223d0af0aaa503)

### Core Innovation

Uses Craig Reynolds's **boids flocking algorithm** to generate spatial trajectories for both granular voices and spectral components. The movement of individual boids in 2D space maps to the spatial positioning of audio elements.

### Boids Algorithm Parameters

The classic Reynolds boids model uses three steering behaviors:
1. **Separation:** Steer to avoid crowding local flockmates
2. **Alignment:** Steer towards the average heading of local flockmates
3. **Cohesion:** Steer to move toward the average position of local flockmates

Each behavior has a weighting factor, and the combination produces emergent flocking behavior.

### Spatial Mapping

- **Boid X position** -> grain azimuth (or speaker channel assignment)
- **Boid Y position** -> grain elevation (or second spatial axis)
- Each boid controls one granular voice, so the flock produces a spatially coherent but organically moving grain cloud

### Implementation

- **Platform:** MaxMSP/Jitter
- **Boid engine:** Eric Singer's Max boids object
- **Visualization:** OpenGL real-time rendering of boid positions
- **Audio applications:**
  1. Granular sampling: boid positions drive spatial locations of granular voices
  2. FFT analysis-resynthesis: boid positions drive spatial locations of individual spectral bins

### Significance

This is an early and influential example of using **emergent behavior algorithms** for spatial granular control. The boids approach produces:
- Organic, non-repetitive spatial movement
- Coherent group behavior (the cloud moves together)
- Individual variation within the group (each grain has slightly different trajectory)
- Controllable "personality" via separation/alignment/cohesion weights

### Plugin Application

A boids-inspired spatial modulation mode could provide:
- `flock_size`: number of spatial trajectory generators
- `separation`: minimum angular distance between grain positions
- `alignment`: tendency to move in same spatial direction
- `cohesion`: tendency to cluster spatially
- `speed`: rate of spatial movement

---

## 9. Spectral and 3D Spatial Granular Synthesis in Csound

**Citation:** Di Liscia, O.P. "Spectral and 3D spatial granular synthesis in Csound." Proceedings of the 6th International Csound Conference (ICSC), 2017.
**Source:** [ICSC2017 Proceedings](https://csound.com/icsc2017/proceedings/ICSC2017_paper_Di.Liscia.pdf)

### Core Concept: Spatial Synthesis of Sound

Di Liscia conceives "Spatial Synthesis of Sound" as a mode where the composer generates sound **together with** its spatial features -- spatial position is not an afterthought but an integral synthesis parameter.

### Dual-Domain Partitioning

The approach partitions the sonic stream in two domains:
1. **Time domain:** Granular synthesis decomposes audio into temporal grains
2. **Frequency domain:** Spectral analysis decomposes audio into frequency bands

Each partition (grain or spectral bin) receives independent spatial treatment.

### Spatial Encoding Methods

The Csound implementation uses:
- **Ambisonics encoding** for per-grain spatial positioning
- **Spatial information retention hypothesis:** When an Ambisonic signal is granulated, the spatial encoding is retained at the granular level -- meaning you can granulate already-spatialized material and the spatial information survives

### Key Technique: Ambisonic Granulation

This is a distinct technique from per-grain Ambisonic encoding:
1. Start with an Ambisonic-encoded source (multi-channel)
2. Apply granular synthesis to ALL Ambisonic channels simultaneously
3. The spatial encoding embedded in the Ambisonic channels is preserved in each grain
4. The granulated output retains spatial characteristics of the original

This suggests a second approach to spatial granular: instead of encoding each grain to a position, granulate an already-spatialized signal.

---

## 10. Efficient HRTF Convolution for Binaural Rendering

### Partitioned Convolution Algorithms

**Primary Reference:** Wefers, F. "Partitioned convolution algorithms for real-time auralization." Ph.D. Dissertation, RWTH Aachen University, 2015.
**Source:** [RWTH Publications](https://publications.rwth-aachen.de/record/466561/files/466561.pdf)

#### The Latency-Computation Tradeoff

Real-time binaural rendering faces a fundamental tradeoff:
- **Small block sizes** = low latency but high computational load
- **Large block sizes** = low computational load but high latency
- Typical real-time audio: 128-512 samples per block (2.7-10.7ms at 48kHz)
- Virtual acoustics demands latency below 10ms

#### Uniformly Partitioned Overlap-Save (UPOLS)

The standard efficient convolution method for HRTF processing:

```
Given:
  - Input block size: B samples
  - HRIR length: M samples
  - Number of partitions: K = ceil(M / B)
  - FFT size: N = 2B (for overlap-save)

Algorithm per audio frame:
  1. Append new B input samples to previous B samples -> 2B samples
  2. FFT of 2B input block -> X(f)
  3. For each partition k = 0..K-1:
     a. Multiply X(f) * H_k(f)  (pre-computed FFT of HRIR partition)
     b. Accumulate result
  4. IFFT of accumulated result -> 2B samples
  5. Take last B samples as output

Computational cost per frame:
  C_UPOLS = (K + 1) * FFT(2B) + K * complex_multiply(2B)
```

#### Non-Uniformly Partitioned Convolution (NUPOLS)

For long impulse responses (room reverb + HRTF), non-uniform partitioning uses:
- **Small first partition** (= audio block size) for minimum latency
- **Progressively larger partitions** for the tail, trading latency for efficiency
- Total cost is significantly lower than uniform partitioning for long filters

```
Example partition scheme for 4096-sample BRIR at 256-sample block size:
  Partition 1: 256 samples (latency = 256 samples = 5.3ms @ 48kHz)
  Partition 2: 256 samples
  Partition 3: 512 samples
  Partition 4: 1024 samples
  Partition 5: 2048 samples
```

#### Per-Source vs. Ambisonics Approach

**Per-source HRTF convolution:**
- Cost: O(S * K) where S = number of sources, K = HRIR partitions
- Best quality (full HRTF per source)
- Linear scaling with source count

**Ambisonics intermediate:**
- Cost: O(S * (L+1)^2) for encoding + O((L+1)^2 * K) for binaural decode
- Source count only affects encoding (cheap multiplication)
- Decoding cost independent of source count
- Quality depends on Ambisonic order

**Crossover point:** Ambisonics becomes more efficient when:
```
S > (L+1)^2 * K_decode / K_hrir
```
For typical values (L=3, K_decode=8, K_hrir=4): S > 32 sources

### GPU Acceleration

**Citation:** "An Efficient Implementation of Parallel Parametric HRTF Models for Binaural Sound Synthesis in Mobile Multimedia." IEEE Trans. Multimedia, 2020.
**Source:** [IEEE Xplore](https://ieeexplore.ieee.org/document/9028216/)

#### Parametric HRTF Model

Rather than storing full HRIR impulse responses, HRTFs are modeled as **parallel second-order IIR filter sections**:

```
H_HRTF(z) = sum_{k=1}^{P} (b0_k + b1_k*z^-1 + b2_k*z^-2) / (1 + a1_k*z^-1 + a2_k*z^-2)
```

- P = number of parallel sections (typically 6-12)
- Each section is a biquad filter
- Interpolation between directions = interpolation of biquad coefficients
- Dramatically reduces storage (from thousands of HRIR samples to ~60-120 coefficients per direction)

#### GPU Implementation (OpenCL)

- Each parallel biquad section maps to one GPU compute unit
- All P sections for one ear computed simultaneously
- Cross-platform via OpenCL framework
- Advantages: reduced computational cost, simple interpolation, small memory footprint

---

## 11. 3D Tune-In Toolkit

**Citation:** Cuevas-Rodriguez, M. et al. "3D Tune-In Toolkit: An open-source library for real-time binaural spatialisation." PLOS ONE, 2019.
**Source:** [PLOS ONE](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0211899)

### Architecture: Decoupled Anechoic + Reverb Processing

```
Per-Source Processing (anechoic path):
  Source mono signal
      |
      v
  Distance attenuation: A(d) = (d_ref/d)^(2 * A_ref / 6)
      |
      v
  Air absorption (>15m): 2x cascaded Butterworth LPF (24 dB/oct)
      |
      v
  Near-field compensation (<2m): ILD corrections
      |
      v
  HRTF convolution (UPOLS per source per ear)
      |
      v
  Stereo output (per source)

Shared Reverb Processing:
  All sources -> 1st-order Ambisonic B-format encoding
      |
      v
  Convolution with Ambisonic BRIRs (reflections only)
      |
      v
  Binaural decode
      |
      v
  Mixed with per-source anechoic signals
```

### HRTF Convolution: Modified UPOLS for Moving Sources

Standard UPOLS assumes static filters. The 3D Tune-In Toolkit adds:

1. **Dedicated delay line** for partitioned HRIR blocks
2. Each new audio frame: shift both delay lines up by one position
3. Insert **new HRIR corresponding to current source position** at position 0
4. This allows HRIR to change every audio frame without full filter recalculation

This is critical for per-grain spatial processing where positions change rapidly.

### HRTF Interpolation: Barycentric on Sphere

```
Given source direction (azim, elev):
  1. Find 3 nearest measured HRIR positions on the HRTF sphere
  2. Calculate distances using Haversine formula:
     d = 2 * r * arcsin(sqrt(sin^2((lat2-lat1)/2) + cos(lat1)*cos(lat2)*sin^2((lon2-lon1)/2)))
  3. Compute barycentric coordinates from the 3 nearest points
  4. Interpolated HRIR = w1*HRIR1 + w2*HRIR2 + w3*HRIR3
```

Advantages:
- Works with irregular HRTF measurement grids
- Smooth transitions between directions
- No assumption about HRTF spatial regularity

### Cross-Ear Parallax Correction

For sources not at the HRTF measurement distance:
- Project source vector onto HRTF measurement sphere **independently for each ear**
- Each ear "sees" a slightly different direction to the source
- Particularly important for near-field sources where head radius is significant relative to source distance

### ITD Processing

ITDs are **extracted and stored separately** from HRIRs to prevent comb-filtering during interpolation:

- **Interpolated ITD:** Barycentric interpolation of measured ITD values
- **Synthesized ITD:** Woodworth formula: `ITD = (r/c) * (theta + sin(theta))` where r = head radius, c = speed of sound, theta = interaural azimuth
- **Smooth transitions:** Stretching/squeezing algorithm resamples signal using linear interpolation to prevent discontinuities during head rotation

### Distance Attenuation Formula

```
A(d) = (d_ref / d) ^ (2 * A_ref / 6)

Where:
  d_ref = reference distance (typically 1m)
  d = source distance
  A_ref = attenuation at reference (default: -6 dB per doubling)

Anechoic path: A_ref = -6 dB (inverse square law)
Reverb path: A_ref = -3 dB (slower reverb decay with distance)
```

Adaptive smoothing: attenuation asymptotically approaches target using exponential law with configurable attack time.

### Far-Field Air Absorption Model (ISO 9613-1)

For distances > 15m:
- Two cascaded 2nd-order Butterworth lowpass filters
- Cutoff frequency decreases exponentially with distance
- 24 dB/octave rolloff approximates atmospheric absorption

---

## 12. HRTF Preprocessing for Ambisonics Rendering

**Citation:** "Assessing HRTF preprocessing methods for Ambisonics rendering through perceptual models." Acta Acustica, 2022.
**Source:** [Acta Acustica](https://acta-acustica.edpsciences.org/articles/aacus/full_html/2022/01/aacus210029/aacus210029.html)

### The Core Problem

Rendering Ambisonics to binaural requires convolving each Ambisonic channel with corresponding HRTF spherical harmonic coefficients. The spatial order of HRTFs is much higher than typical Ambisonic signals, creating a mismatch that degrades quality.

### Key Formula: Aliasing Frequency

```
f_a = (N_a * c) / (2 * pi * r)

Where:
  N_a = Ambisonic order
  c = speed of sound (343 m/s)
  r = head radius (0.0875 m)

Examples:
  Order 1: f_a ~  625 Hz
  Order 3: f_a ~ 1875 Hz
  Order 5: f_a ~ 3125 Hz
  Order 7: f_a ~ 4375 Hz
```

Below the aliasing frequency, Ambisonic reproduction is accurate. Above it, spatial aliasing occurs.

### Nine HRTF Preprocessing Methods Ranked

From best to worst overall performance:

1. **BiMagLS (Bilateral Magnitude Least Squares):** Best overall -- time-aligns HRTF, least-squares below 3kHz, magnitude-only fitting above with iterative phase estimation. Requires bilateral Ambisonics (separate L/R ear signals).

2. **MagLS (Magnitude Least Squares):** Best for standard Ambisonics -- sets phase to zero above aliasing frequency, preserves ITDs below. Smooth transition at crossover.

3. **TA (Time-Alignment):** Phase correction by ear alignment before SH truncation. Requires bilateral implementation.

4. **SpSubMod:** Combines frequency-dependent time-alignment + dual-band tapering + HRF equalization with spatial subsampling.

5. **SpSub (Spatial Subsampling):** Samples high-order SH-HRTF to Nth order via Gaussian quadrature. Intentionally introduces spatial aliasing to compensate truncation errors.

6. **Tap (Tapering):** Hann windowing on highest 3 orders + dual-band processing above aliasing frequency + HRF equalization.

7. **EQ (Equalization):** HRF-based diffuse field matching with frequency-dependent regularization.

8. **Trunc (Truncation):** Baseline -- simply removes SH coefficients beyond order N. Worst performance but simplest.

9. Method variants of the above with specific configurations.

### Convergence Behavior

- All methods converge toward reference quality at order 20+
- Differences among methods are **relatively large for orders 1-5** and diminish above order 10
- Above order 30, differences fall below 0.03 sones PSD (perceptually negligible)

### Practical Recommendation for Plugin

For a binaural granular plugin using Ambisonics as intermediate:
- **Order 3 with MagLS preprocessing:** Best quality/cost balance for standard Ambisonics
- **Order 1 with MagLS:** Minimum viable, acceptable for ambient/textural content
- **Order 5+:** Diminishing returns for most granular textures (which are inherently diffuse)

---

## 13. Steam Audio: Production Binaural Pipeline

**Source:** [Steam Audio C API Guide](https://valvesoftware.github.io/steam-audio/doc/capi/guide.html)

### Per-Source Binaural Processing

Each source gets an `IPLBinauralEffect` maintaining internal state across frames:

```
Parameters per source per frame:
  - direction: Vec3 (source relative to listener in listener coordinates)
  - interpolation: NEAREST or BILINEAR
  - spatialBlend: 0.0 (unspatialized) to 1.0 (fully spatialized)
  - hrtf: IPLHRTF reference (changeable per frame)
```

### HRTF Interpolation Methods

1. **Nearest-neighbor:** Select closest measured direction. Fastest but audible transitions for moving sources.
2. **Bilinear:** Blend among 4 closest directions. Slower but significantly smoother for moving sources.

### Ambisonics Pipeline

```
Source (mono) -> IPLAmbisonicsEncodeEffect -> (N+1)^2 channels
                                                   |
                                                   v
                            IPLAmbisonicsDecodeEffect -> Binaural (HRTF) or Speaker Layout
```

- Ambisonic rotation via orientation matrices (no reprocessing needed)
- Encoding cost: O(S * (N+1)^2) multiplications per frame
- Decoding cost: O((N+1)^2) convolutions per frame (independent of source count)

### Direct Effect Chain (Pre-Binaural)

Before HRTF convolution, each source undergoes:
1. **Distance attenuation** (inverse square law or custom curve)
2. **Air absorption** (3-band EQ model)
3. **Directivity** (source radiation pattern)
4. **Occlusion/Transmission** (frequency-dependent blocking)

### Performance Optimizations

- **SIMD:** Auto-selects AVX2, SSE, or NEON instruction sets
- **Multi-threading:** Simulation on separate thread; effects applied on audio thread
- **Buffer sizing:** Configurable 512-1024 samples (typical)
- **SOFA file support:** Custom HRTF datasets

---

## 14. NIME 2025: Per-Grain Parameterisation

**Citation:** "Towards Per-Grain Parameterisation in Granular Synthesis." Proceedings of NIME 2025.
**Source:** [NIME 2025 Proceedings](https://nime.org/proceedings/2025/nime2025_21.pdf)

### Design Philosophy

Integrates atomic (per-grain) parameterisation with modulation controls at the instrument level. The key tension: per-grain control is compositionally powerful but can overwhelm performers without good modulation routing.

### Modulation Architecture

- **LFOs:** Routable to all grain engine and per-grain effect parameters
- **XY Macro Controls:** Provide high-level gestural control that maps to multiple per-grain parameters simultaneously
- **Combined approach:** XY macros for performance, direct per-grain for composition

### Per-Grain Effect Parameters

Each grain receives independent effect processing with controllable parameters, extending beyond synthesis parameters to include spatial position as a per-grain effect.

---

## 15. Implementation Strategy Synthesis

### Architecture Recommendations for a JUCE Spatial Granular Plugin

Based on the compiled research, here are the key architectural decisions:

#### A. Spatial Encoding Strategy: Hybrid Approach

**Recommended:** Two-tier architecture combining per-grain panning with Ambisonic intermediate

```
Tier 1: Per-Grain Amplitude Panning (lightweight)
  - Each grain gets (azimuth, elevation, distance) parameters
  - VBAP or equal-power panning for speaker feeds
  - Computational cost: ~10 multiplications per grain
  - Suitable for: up to 2048+ simultaneous grains (proven by EC2)

Tier 2: Ambisonic Encoding (higher quality, binaural-ready)
  - N parallel grain streams (8-32), each HOA-encoded
  - Each stream: mono -> (L+1)^2 channels via SH encoding
  - Streams summed in HOA domain
  - Decoded to binaural (MagLS HRTF) or speaker layout
  - Recommended order: 3rd (16 channels, good quality/cost balance)
```

#### B. Critical Perceptual Parameters

From Riedel et al. (2023):
- **Temporal fusion threshold: Delta-t < 20ms** for continuous spatial texture
- **Above 20ms:** discrete spatial events (useful for rhythmic spatial patterns)
- **Height channels:** contribute to engulfment (overhead sensation), not envelopment (surround)
- **Lowpass filtering:** enhances envelopment perception

From Rossetti & Manzolli (2020):
- **Higher Ambisonic orders** enhance decorrelation and perceived timbral richness
- **3rd order minimum** for meaningful spatial granular textures

#### C. Binaural Rendering Strategy

From 3D Tune-In Toolkit and Wefers:

```
For per-grain binaural (high quality, high cost):
  - UPOLS convolution with modified delay line for source movement
  - Barycentric HRTF interpolation (3 nearest directions)
  - Separate ITD processing (Woodworth formula or measured)
  - Practical limit: ~32-64 simultaneous HRTF convolutions at 48kHz/256 block

For grain cloud binaural (efficient, scalable):
  - Encode grains to 3rd-order Ambisonics (16 channels)
  - Single binaural decode of the Ambisonic bus
  - MagLS preprocessing for HRTF
  - Scales to unlimited grain count (encoding is cheap)
```

#### D. Spatial Distribution Modes (from literature)

1. **Spherical Cap** (IEM GranularEncoder): Size + Shape parameters
2. **Spherical Shell Sector** (Ambitools): Azimuth/elevation/radius ranges
3. **Boids Flocking** (Kim-Boyle): Emergent organic movement via separation/alignment/cohesion
4. **Uniform Random** (Riedel et al.): Maximum envelopment sensation
5. **Trajectory-based:** Pre-defined or LFO-driven spatial paths per grain stream

#### E. Per-Grain Signal Chain (from EC2 + IEM + literature)

```
1. Source Selection (buffer position + pitch)
2. Grain Envelope (window function)
3. Per-Grain Filter (bandpass with center freq + Q)
4. Per-Grain Amplitude
5. Spatial Encoding:
   a. Position assignment (azimuth, elevation, distance)
   b. VBAP gains OR Ambisonic SH coefficients
   c. Optional: HRTF convolution (binaural mode)
6. Distance processing (attenuation + air absorption + near-field ILD)
7. Output to spatial bus
```

#### F. Computational Budget Estimates

At 48kHz, 512-sample buffer:

| Operation | Cost per Grain | Max Grains (single core) |
|-----------|---------------|--------------------------|
| Grain synthesis + envelope | ~20 multiply-adds/sample | ~2000 |
| Per-grain stereo pan | ~4 multiply-adds/sample | ~5000 |
| Per-grain VBAP (8ch) | ~16 multiply-adds/sample | ~3000 |
| Per-grain 3rd-order HOA encode | ~32 multiply-adds/sample | ~1500 |
| Per-grain HRTF (UPOLS, 128-tap) | ~500 multiply-adds/sample | ~100 |
| Per-stream 3rd-order HOA encode | ~32 multiply-adds/sample | 32 streams = unlimited grains |

**Key insight:** Per-grain HRTF convolution is ~50x more expensive than per-grain HOA encoding. The Ambisonics intermediate approach (encode per-grain/stream, decode once) is strongly recommended for binaural output.

---

## Source Bibliography

1. **Granular Spatialisation (ICMC 2016):** [University of Michigan Digital Library](https://quod.lib.umich.edu/cgi/p/pod/dod-idx/granular-spatialisation-a-new-method-for-sound-diffusion.pdf?c=icmc&idno=bbp2372.2016.006&format=pdf)
2. **Lecomte & Fernandez - Ambitools (IFC 2024):** [HAL Archives](https://hal.science/hal-04846653v1)
3. **Roads, Kilgore, DuPlessis - EC2 (CMJ 2021):** [MIT Press](https://direct.mit.edu/comj/article-abstract/45/3/20/113899/)
4. **Riedel, Frank, Zotter - Envelopment (JAES 2023):** [arXiv:2301.10210](https://arxiv.org/abs/2301.10210) | [GitHub](https://github.com/stefanriedel/JAES_SpatialGranularSynthesis)
5. **Rossetti & Manzolli - Perception in HOA (OPUS 2020):** [OPUS Journal](https://www.anppom.com.br/revista/index.php/opus/article/view/opus2020b2610)
6. **IEM GranularEncoder:** [IEM Plugin Suite](https://plugins.iem.at/docs/granularencoder/)
7. **RingBuffer - SC Spatial Granular:** [RingBuffer](http://ringbuffer.org/spatial_audio/spatial_synthesis/spatial_granular_supercollider/)
8. **Kim-Boyle - Boids (ICMC 2006):** [Semantic Scholar](https://www.semanticscholar.org/paper/Spectral-and-Granular-Spatialization-with-Boids-Kim-Boyle/687c5a827614027c4a9199708a223d0af0aaa503)
9. **Di Liscia - Csound (ICSC 2017):** [ICSC Proceedings](https://csound.com/icsc2017/proceedings/ICSC2017_paper_Di.Liscia.pdf)
10. **Wefers - Partitioned Convolution (2015):** [RWTH Aachen](https://publications.rwth-aachen.de/record/466561/files/466561.pdf)
11. **Cuevas-Rodriguez et al. - 3D Tune-In (PLOS ONE 2019):** [PLOS ONE](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0211899)
12. **HRTF Preprocessing for Ambisonics (Acta Acustica 2022):** [Acta Acustica](https://acta-acustica.edpsciences.org/articles/aacus/full_html/2022/01/aacus210029/aacus210029.html)
13. **Steam Audio:** [Valve Software](https://valvesoftware.github.io/steam-audio/doc/capi/guide.html)
14. **GPU Parametric HRTF (IEEE 2020):** [IEEE Xplore](https://ieeexplore.ieee.org/document/9028216/)
15. **NIME 2025 - Per-Grain Parameterisation:** [NIME Proceedings](https://nime.org/proceedings/2025/nime2025_21.pdf)
16. **Thewolfsound - Fast Convolution:** [WolfSound](https://thewolfsound.com/fast-convolution-fft-based-overlap-add-overlap-save-partitioned/)
