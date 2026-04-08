# Voice Synthesis Realism: Comprehensive Research

Research for improving O-Formant (JUCE-based formant synthesizer plugin).
Covers academic literature, commercial analysis, and perceptual science.

---

## 1. Glottal Source Models

### 1.1 The LF (Liljencrants-Fant) Model

The LF model (Fant, Liljencrants & Lin, 1985) is the gold standard for parametric glottal flow modeling. It represents the derivative of glottal flow as a piecewise function:

```
u(n) = E1 * exp(lambda * n * Ts) * sin(omega * n * Ts)    [0 <= nTs < Te]
u(n) = -E2 * [exp(-mu*(nTs-Te)) - exp(-mu*(Tc-Te))]       [Te <= nTs < Tc]
u(n) = 0                                                    [Tc <= nTs <= T0]
```

Six descriptive parameters:
- **T0**: Glottal vibration period (1/f0)
- **Tp**: Instant of maximum glottal flow (open phase peak)
- **Te**: Instant of maximum negative flow derivative (closing instant)
- **Ta**: Duration of return phase (controls abruptness of closure)
- **Tc**: Instant of complete glottal closure (often = T0)
- **Ee**: Amplitude at glottal closure instant

Four derived synthesis coefficients (E1, lambda, mu, omega) are computed from descriptive parameters via constraint equations including a zero-mean condition on the flow derivative.

**The Rd parameter** (vocal effort): A single-parameter simplification that captures the key perceptual dimension of the LF model. Rd ranges from 0.3 (pressed/tense) to 2.7 (breathy/relaxed), with ~1.0 being modal voice. This is the approach used by the GOLF vocoder and O-Formant's current wavetable implementation.

**Actionable for O-Formant**: The current LF wavetable with Rd interpolation (100 Rd steps x 2048 samples, bilinear interpolation across Rd and mipmap levels) follows the GOLF paper's approach and is well-suited. The key improvement opportunity is in how Rd is modulated -- it should vary dynamically with pitch, intensity, and expressive intent rather than being a static knob.

### 1.2 KLGLOTT88 (Klatt & Klatt, 1990)

A simplified glottal model used in the Klatt synthesizer. Consists of a Rosenberg-type waveform plus a first-order lowpass filter controlling spectral tilt. Parameters:
- **Oq**: Open quotient (duration of open phase as % of T0)
- **TL**: Spectral tilt in dB (lowpass cutoff controls closure smoothness)
- **AV**: Voicing amplitude

Simpler than LF but less flexible. The spectral tilt parameter is perceptually important.

### 1.3 Rosenberg Model

The simplest common glottal model -- a half-cosine opening phase and cosine-squared closing phase. Used as the basis for KLGLOTT88. Perceptual evaluation studies (Kreiman et al., 2015, JASA) found it produces the worst perceptual match to natural voices among tested models, significantly worse than LF.

### 1.4 Unified Parameter Framework

A unified set of 5 parameters describes all common glottal models (LF, KLGLOTT88, Rosenberg): fundamental period, maximum excitation, open quotient, asymmetry coefficient, and return-phase quotient. This enables comparison and translation between models.

### 1.5 Critical Perceptual Finding: Spectral Match > Time-Domain Fit

Kreiman et al. (2015) found that time-domain waveform accuracy does NOT predict perceptual quality. The spectral match in ranges H1-H2, H2-H4, H4-2kHz, and H1-Hn predicted 42% of variance in perceived quality. The timing and amplitude of the negative peak of the flow derivative (closing phase) correlated most with listener preference, not the opening phase.

**Actionable for O-Formant**: Optimize glottal source for spectral characteristics, not waveform shape. The H1-H2 difference (spectral tilt) is the single most important parameter for voice quality perception.

---

## 2. Voice Quality Control via Spectral Tilt (H1-H2)

The difference between the first and second harmonics (H1-H2) is the primary acoustic correlate of phonation type:

| Phonation Type | H1-H2 (typical) | Open Quotient | Characteristics |
|---------------|-----------------|---------------|-----------------|
| Pressed/Creaky | Low (< 0 dB) | ~0.3-0.4 | Sharp closure, strong harmonics, low noise |
| Modal | Mid (~2-6 dB) | ~0.5 | Balanced closure, moderate harmonic slope |
| Breathy | High (> 8 dB) | ~0.6-0.8 | Gradual closure, steep spectral tilt, more noise |

Key relationships:
- H1-H2 correlates with glottal constriction degree
- Lower H1-H2 = more constricted (pressed)
- Higher H1-H2 = less constricted (breathy)
- H1-H2 does NOT depend on vowel quality (fixed harmonic bandwidths)
- H1-H2 clearly distinguishes breathy from modal in both sexes
- H1-H2 distinguishes creaky from modal primarily in males

**Actionable for O-Formant**: The Rd parameter already controls this continuum. Map Rd to clear perceptual labels (pressed/modal/breathy) in the UI. Consider exposing H1-H2 as a more intuitive control than raw Rd.

---

## 3. Micro-Perturbations: Jitter, Shimmer, and HNR

### 3.1 Jitter (Frequency Perturbation)

Cycle-to-cycle variation in fundamental period:
```
Jitter(relative) = (1/(N-1)) * SUM(|T_i - T_{i+1}|) / mean(T)
```

Normal healthy values: < 1% relative jitter (pathological threshold ~1.04%)
Typical implementation: Random perturbation of f0 on each glottal cycle.

Jitter decreases with increasing f0 and increasing vocal intensity. For natural-sounding synthesis, jitter should be inversely scaled with pitch and loudness.

### 3.2 Shimmer (Amplitude Perturbation)

Cycle-to-cycle variation in peak amplitude:
```
Shimmer(relative) = (1/(N-1)) * SUM(|A_i - A_{i+1}|) / mean(A)
```

Normal healthy values: < 3% relative shimmer (pathological threshold ~3.81%)
Shimmer(dB) threshold: ~0.35 dB

### 3.3 Harmonics-to-Noise Ratio (HNR)

Normal: 15-20 dB or higher. Pathological: below 7 dB.
Females typically have slightly higher HNR than males.

### 3.4 Implementation Strategy for O-Formant

**Current state**: O-Formant has no jitter/shimmer modeling.

**Recommended implementation**:
1. Per-cycle random jitter: Apply small random offset to phase increment each glottal cycle. Use 1/f noise (not white noise) for more natural variation patterns.
2. Per-cycle random shimmer: Modulate glottal pulse amplitude each cycle.
3. Scale perturbation amounts inversely with pitch and intensity.
4. Typical ranges for musical use: 0-2% jitter, 0-5% shimmer (wider than clinical norms for expressivity).
5. Aspiration noise already serves as a partial proxy for low HNR, but explicit control of the noise-to-harmonic balance would add realism.

---

## 4. Formant Synthesis Architecture

### 4.1 The Klatt Synthesizer (1980)

Dennis Klatt's cascade/parallel formant synthesizer remains the most influential formant synthesis architecture. Key design features:

**Dual pathway**:
- **Cascade section**: Chain of 5-6 second-order bandpass filters in series. Used for vowels and sonorants. Formant amplitudes are determined by resonator interaction -- only frequencies and bandwidths need specification.
- **Parallel section**: Bank of bandpass filters in parallel, each with independent amplitude control. Used for fricatives, plosive bursts, and nasals where anti-resonances or specific spectral shapes are needed.

**40 parameters per frame** (updated every 5-10ms):
- Voice source: f0, AV (voicing amplitude), AH (aspiration), TL (spectral tilt), OQ (open quotient)
- Cascade formants: F1-F6 frequencies, B1-B6 bandwidths
- Parallel formants: A1-A6 amplitudes, AF (frication), AN (nasal)
- Nasal: FP (nasal pole freq), FZ (nasal zero freq), BP/BZ (bandwidths)

**Key insight**: The cascade configuration naturally produces correct relative formant amplitudes for vowels -- you only need to set frequencies and bandwidths. The parallel configuration requires explicit amplitude control but can produce the spectral zeros needed for consonants and nasals.

**Actionable for O-Formant**: The current architecture uses parallel bandpass filters (like the Klatt parallel section). For improved vowel naturalness, consider a cascade (series) configuration for the vowel path. The parallel path is correct for consonant noise shaping.

### 4.2 Formant Frequencies and Bandwidths

Typical formant ranges for adult male voice (17cm vocal tract):

| Formant | Frequency Range | Typical Bandwidth |
|---------|----------------|-------------------|
| F1 | 200-1200 Hz | 50-140 Hz |
| F2 | 600-2800 Hz | 62-149 Hz |
| F3 | 1800-3500 Hz | 67-223 Hz |
| F4 | 3200-4500 Hz | ~200-300 Hz |
| F5 | 3800-5500 Hz | ~300-500 Hz |

Female formants are ~15-20% higher; children's are ~30-50% higher.
Female bandwidths are slightly wider than male.

**F1 correlates with vowel height** (open/close): low F1 = close vowel [i,u], high F1 = open vowel [a]
**F2 correlates with vowel frontness**: high F2 = front [i,e], low F2 = back [u,o]

### 4.3 Formant Bandwidth and Naturalness

Bandwidth is a critical but often underappreciated parameter. It is determined by:
1. Viscous and thermal losses along vocal tract walls
2. Radiation losses at the lip opening
3. Mechanical impedance of vocal tract tissue
4. Glottal opening (wider opening = wider B1 bandwidth)

Research (Fleischer et al., 2015) shows that formant frequencies and bandwidths cannot be modeled correctly without considering vocal tract wall impedance. The wall acts as a distributed mass-damping system that:
- Shifts formant frequencies downward from rigid-wall predictions
- Increases formant bandwidths (especially F1)
- Shows frequency-dependent behavior varying with vowel and geometry

**Actionable for O-Formant**: The current fixed bandwidth values (40-130 Hz) are reasonable starting points but should vary with:
- Vowel type (open vowels have wider B1 due to acoustic coupling with glottal opening)
- Vocal effort/loudness (higher effort = slightly narrower bandwidths due to firmer tissues)
- Breathiness (breathier voice = wider bandwidths, especially B1)

---

## 5. Formant Transitions and Coarticulation

### 5.1 Perceptual Importance

Formant transitions are critical perceptual cues:
- **F1 transitions**: Cue manner of articulation (stop vs. fricative vs. approximant)
- **F2 and F3 transitions**: Cue place of articulation (labial vs. alveolar vs. velar)

Research (Nearey & Assmann, 1986; Strange, 1989) confirms that both the steady-state vowel portion and the formant transitions contribute to vowel identification.

### 5.2 Perceptual Tolerance

Benders et al. (2010, JASA) found significant perceptual tolerance for imperfect transitions:
- Delays between F1 and F2 transition onsets up to **30ms** are not perceived
- Duration differences up to **40ms** tolerated when transitions start/end simultaneously
- Symmetric transitions tolerate up to **50ms** differences
- Most deviations from linearity and synchrony in natural speech are perceptually insignificant

This is good news for real-time synthesis -- perfectly synchronized, smooth transitions are sufficient.

### 5.3 Coarticulation Modeling (Birkholz et al., 2013)

A weighted-average approach for consonant-vowel coarticulation:
- Measure consonant vocal tract shapes in context of 3 corner vowels (/a/, /i/, /u/)
- For any target vowel, interpolate consonant shape using bilinear weighting
- Achieved 82.4% consonant recognition rate in synthesized CV syllables

**Actionable for O-Formant**: The current VowelMorpher provides static vowel targets. For more realistic transitions:
1. Implement formant interpolation with asymmetric timing (faster F1 transitions for stops, slower for approximants)
2. Add "target undershoot" -- formants approach but don't fully reach targets in rapid speech, scaled by speech rate
3. Transition durations: 20-50ms for stops, 50-100ms for fricatives, 80-150ms for glides

---

## 6. Nasalization

### 6.1 Acoustic Effects

Coupling the nasal cavity to the oral tract via velopharyngeal opening introduces:
1. **Nasal poles** (resonances): Additional spectral peaks, especially around 250-300 Hz (nasal murmur frequency) and ~1000 Hz
2. **Nasal zeros** (anti-resonances): Spectral troughs that cancel nearby oral formants
3. **F1 broadening**: Wider first formant bandwidth due to added damping
4. **Overall amplitude reduction**: Energy splitting between oral and nasal radiation

### 6.2 Pole-Zero Implementation

The Klatt synthesizer models nasalization with:
- **FP**: Nasal pole frequency (~300 Hz default)
- **FZ**: Nasal zero frequency (between FP and F1)
- **BP, BZ**: Nasal pole/zero bandwidths (~90 Hz each)

Each sinus cavity (maxillary, sphenoidal) introduces additional pole-zero pairs.
The nasal tract asymmetry between left and right passages adds further pole-zero pairs.

**Actionable for O-Formant**: Add a nasalization parameter that:
1. Introduces a pole-zero pair into the formant filter chain
2. Pole at ~250-350 Hz (nasal murmur), zero slightly above F1
3. Widens B1 proportional to nasalization amount
4. Reduces overall output amplitude slightly (~2-3 dB at full nasalization)

---

## 7. Source-Filter Interaction (Nonlinear Coupling)

### 7.1 Theory (Titze, 2008, JASA)

The classical source-filter model assumes independence between glottal source and vocal tract. In reality, significant nonlinear coupling exists:

**Control parameter**: The ratio ag/A* (mean glottal area / epilarynx tube area). Narrowing the epilarynx tube from 3.0 cm^2 to 0.2 cm^2 dramatically increases coupling.

**Key effects**:
1. **Inertive reactance skews glottal flow pulse**: Positive (inertive) supraglottal reactance below F1 enhances vocal fold vibration amplitude and skews the flow pulse, creating harmonics not present in the uncoupled source.
2. **Efficiency gains**: Glottal efficiency increases from ~2.4% (no coupling) to ~5.5% (strong coupling), with radiated acoustic power increasing tenfold.
3. **F0 instabilities near formant crossings**: When a harmonic approaches F1, rapid reactance changes can cause 30-40 Hz pitch jumps and register transitions.
4. **New harmonic distortion frequencies**: The vocal tract creates frequencies absent from the source -- challenging the linear source-filter assumption.

### 7.2 Implications for Synthesis

Two levels of interaction:
- **Level 1**: Flow-pressure coupling (tractable -- model as formant-dependent gain modulation)
- **Level 2**: Mode-of-vibration changes (complex -- produces subharmonics, bifurcations)

**Actionable for O-Formant**: A practical approximation:
1. Boost harmonic energy when f0 harmonics are near formant frequencies (simulates inertive loading)
2. Add slight f0 instability (jitter increase) when harmonics cross formant frequencies
3. Increase glottal pulse skewness (reduce Rd slightly) when source is strongly loaded by narrow vocal tract

---

## 8. The Singer's Formant

### 8.1 Characteristics

A prominent spectral peak at 2.5-3.5 kHz found in trained operatic singers (especially male). Enables projection over orchestral accompaniment.

**Acoustic mechanism**: Clustering of F3, F4, and F5 into a single reinforced peak.

**Anatomical conditions** (Sundberg):
1. Pharynx cross-section at least 6x wider than epilarynx tube opening
2. Wide sinus morgagni (tunes extra resonance between F3 and F4)
3. Piriform sinuses act as side branches affecting F5

### 8.2 The Piriform Fossae

Two cavities lateral to the laryngeal vestibule introducing:
- Anti-resonance at ~4-5 kHz
- "Formant repellent" effect pushing nearby formants lower
- Spectral enhancement of the singer's formant cluster region

Perceptual studies show mixed results -- piriform fossae make synthesis more acoustically realistic but can reduce perceived naturalness due to emphasized low frequencies creating a muffled quality. A tense voice source partially compensates.

**Actionable for O-Formant**: For singing mode, allow F3-F5 to be manually narrowed into a cluster around 3 kHz. This is simpler than modeling the anatomical mechanism but achieves the perceptual effect.

---

## 9. Vibrato Modeling

### 9.1 Parameters

Natural singing vibrato characteristics:
- **Rate**: 5-8 Hz (typically ~6 Hz)
- **Extent**: +/- 1-2 semitones (wider in opera, narrower in pop)
- **Onset delay**: Vibrato typically begins 200-400ms after note onset
- **Three modulation components**: Frequency modulation (dominant in voice), amplitude modulation, and spectral envelope modulation
- Listeners show significant preference for sounds with modulated spectral envelope (p < 0.001)

### 9.2 Humanization

Natural vibrato is NOT a perfect sine wave:
- Rate varies slightly over time (wobble)
- Extent varies with intensity and pitch
- Slight asymmetry (often wider on the sharp side)
- Correlation with amplitude fluctuation (tremolo)

**Actionable for O-Formant**: The current VibratoLFO should include:
1. Rate jitter (slow random modulation of vibrato rate, +/- 0.5 Hz)
2. Extent modulation tied to MIDI velocity/expression
3. Onset delay (ramping vibrato depth from 0 to full over 200-400ms after note start)
4. Spectral envelope modulation: formant frequencies should shift slightly with vibrato (they naturally do due to subglottal pressure changes)

---

## 10. Consonant Synthesis

### 10.1 Place of Articulation (Spectral)

Consonant noise spectra are shaped by the constriction location:
- **Labial** (/p,b,m/): Flat or slightly falling spectrum, energy spread across frequencies
- **Alveolar** (/t,d,n,s/): High-frequency concentration, peak around 3-8 kHz
- **Palatal** (/sh,ch/): Peak around 2.5-4 kHz
- **Velar** (/k,g/): Compact burst spectrum, peak frequency varies with following vowel (1.5-4 kHz)

### 10.2 Manner of Articulation (Temporal)

- **Plosives**: Silence (closure) -> burst (2-10ms) -> aspiration/VOT (0-80ms) -> vowel
- **Fricatives**: Gradual onset (20-40ms) -> sustained noise -> gradual offset
- **Affricates**: Plosive burst -> fricative continuation
- **Nasals**: Low-frequency nasal murmur (~250 Hz) during closure, formant transitions at release
- **Approximants** (/l,r,w,j/): Smooth formant transitions, minimal noise

### 10.3 Voice Onset Time (VOT)

Critical for distinguishing voiced/voiceless stops:
- Voiced (/b,d,g/): VOT ~0-20ms (voicing during or just after burst)
- Voiceless unaspirated (/p,t,k/ in some languages): VOT ~20-40ms
- Voiceless aspirated (/p,t,k/ in English): VOT ~40-80ms

**Actionable for O-Formant**: The current ConsonantEngine models place (spectral center) and manner (plosive-to-fricative continuum) well. Missing elements:
1. VOT modeling (delay between burst and voicing onset)
2. Nasal murmur generation (low-frequency resonance during nasal consonant closures)
3. Formant transitions that differ by place of articulation (F2 locus: labial ~700 Hz, alveolar ~1800 Hz, velar ~variable)
4. Burst spectrum that varies with following vowel context (especially for velars)

---

## 11. Subglottal Resonances

### 11.1 Characteristics

The subglottal system (trachea, bronchi, lungs) has its own resonances:
- First subglottal resonance (Sg1): ~600-700 Hz
- Second subglottal resonance (Sg2): ~1300-1500 Hz
- Third subglottal resonance (Sg3): ~2100-2300 Hz

These create spectral zeros in the voice output near the subglottal resonance frequencies.

### 11.2 Perceptual Role

Subglottal resonances affect:
- F1 production (interaction near Sg1 causes discontinuities)
- Register transitions (chest vs. head voice correlates with coupling above/below Sg1)
- Speaker identification (subglottal resonances are relatively invariant per speaker)

**Actionable for O-Formant**: Low priority for initial improvement. Could add subtle spectral zeros near 600 Hz and 1400 Hz to increase naturalness, but the effect is subtle compared to other improvements.

---

## 12. Commercial and Open-Source Analysis

### 12.1 Vocaloid (Yamaha)

**Technique**: Frequency-domain concatenative synthesis. Breaks words into diphone sequences, selects samples from a singer library, concatenates in spectral domain.

**Key details**:
- Japanese requires ~500 diphones per pitch, English requires ~2500
- 3-4 pitch ranges recorded per voice to cover range naturally
- VOCALOID6 (2022) added AI synthesis engine using deep learning for more expressive output

**What makes it realistic**: Large recorded sample database, multiple pitch layers, frequency-domain crossfading. Weakness: Concatenation artifacts at diphone boundaries, limited real-time capability.

### 12.2 Synthesizer V (Dreamtonics)

**Technique**: Hybrid concatenative + deep neural network synthesis. The DNN generates expressive variations from recorded training data.

**Key features**:
- Cross-lingual: AI voices sing in English, Japanese, Chinese regardless of training language
- Deep learning inference produces near-indistinguishable-from-human quality
- Available as VST3/AU plugin for DAW integration
- Live Rendering mode for real-time preview

**Relevance to O-Formant**: Demonstrates that pure parametric synthesis cannot match sample-based approaches for realism. However, parametric synthesis offers unique advantages in real-time control and novel sound design that concatenative cannot.

### 12.3 Plogue Alter/Ego and Chipspeech

**Technique**: Real-time formant synthesis via the ARiA engine. Multiple synthesis algorithms:
- **Formant Singer**: Monopitched formant synthesis with wide vocal range (used by Dandy 704, Lady Parsec, Bert Gotrax)
- **Resynthesis**: Analysis-resynthesis of recorded voice with formant/partial DSP (Daisy voice)

**Key advantage**: True real-time, zero-latency synthesis as a VST/AU plugin. The closest existing product to what O-Formant aims to be.

**Relevance**: Demonstrates that real-time formant singing synthesis is commercially viable. The formant singer method trades quality for control range and real-time performance.

### 12.4 Pink Trombone (Neil Thapen)

**Technique**: 1D digital waveguide articulatory synthesis in JavaScript/Web Audio.

**Implementation details**:
- Bidirectional delay line with ~44 sections representing the vocal tract
- Reflection coefficients: `(d1 - d2) / (d1 + d2)` at section boundaries based on diameter changes
- Separate nasal tract running in parallel
- Turbulence noise injected at constriction points (when diameter 0 < d < 0.3)
- Glottis: Phase-accumulation oscillator with tenseness parameter controlling waveform shape and noise injection
- Dual-pass processing per sample (half-sample offset) for numerical stability
- Tract length: 15.2 cm default
- Vibrato: ~5 Hz default

**Relevance to O-Formant**: Demonstrates that a simple waveguide model produces convincing vowels and basic consonants in real-time. The key insight is that the continuous diameter array (rather than discrete formant frequencies) allows smooth, physically realistic transitions. However, the formant approach (O-Formant's current architecture) gives more direct control over specific vowel targets.

### 12.5 WORLD Vocoder (Morise et al., 2016)

**Technique**: Analysis-synthesis vocoder with three components:
- **DIO/Harvest**: F0 estimation
- **CheapTrick**: Spectral envelope estimation
- **D4C**: Aperiodicity estimation

Achieves high quality at 10x real-time speed. Open source (C++).

**Relevance**: WORLD's decomposition into F0 + spectral envelope + aperiodicity is conceptually similar to O-Formant's source-filter model. The aperiodicity estimation could inform better breathiness/noise modeling.

### 12.6 GOLF Vocoder (Yu & Fazekas, 2024)

**Technique**: Neural source-filter synthesis using:
- Pre-computed LF glottal flow wavetables (100 Rd steps x 2048 samples)
- Frame-wise LPC filtering with overlap-add (not sample-by-sample)
- Cascaded biquad IIR sections for stability
- 22nd-order LPC filter (11 biquads)
- 200 Hz frame rate, 24 kHz sample rate

**Performance**: 0.023 real-time factor on CPU, 14.5-21% of competing systems' parameter count.

**Relevance to O-Formant**: The LF wavetable approach is directly adopted in O-Formant's current LFGlottalSource. The overlap-add LPC filtering approach could replace the current parallel bandpass bank for improved naturalness.

### 12.7 eCantorix

**Technique**: Frontend for eSpeak NG, which uses Klatt formant synthesis. Adjusts pitch and duration of speech samples for singing. Open source (GPLv3).

**Relevance**: Minimal -- primarily a wrapper around existing TTS, not a real-time synthesis engine.

### 12.8 VocalTractLab (Birkholz)

**Technique**: 3D geometric articulatory model with 23 degrees of freedom controlling wireframe meshes of articulators. Acoustic simulation via either 1D reflection-type analog or 3D multimodal frequency-domain method.

**Relevance**: Research reference for how articulatory parameters map to acoustic output. Not real-time, but the mapping data (articulator positions to formant frequencies) could inform O-Formant's parameter ranges.

---

## 13. The Vocal Uncanny Valley

### 13.1 Key Findings

Research on voice uncanniness reveals counterintuitive results:

**Deviation, not ambiguity, drives uncanniness** (Koudenburg et al., 2024): Voices that deviate from typical human patterns trigger negative evaluations. Categorization ambiguity (confusion about whether a voice is human or synthetic) does NOT predict uncanniness.

**Synthetic voices escape the uncanny valley**: Unlike visual uncanny valley effects, clearly synthetic voices are not perceived as eerie. The uncanny valley is triggered by distorted or pathological human-like voices, not by obviously artificial ones.

**Higher human-likeness = lower eeriness** (Kuhne et al., 2020, Frontiers): As synthesized voices become more human-like, they are rated as LESS eerie, not more. This contradicts the traditional uncanny valley hypothesis for voices.

### 13.2 What Listeners Detect as Synthetic

Four diagnostic features (Kuhne et al., 2020):
1. **Intonation problems**: "Too perfect," "metallic," "unnatural pauses"
2. **Sound quality**: "Flat," "choppy," "monotonous"
3. **Emotion-content mismatch**: Incongruence between emotional tone and semantic content
4. **Missing embodiment cues**: No breathing sounds, no sense of physical space, no mouth-to-microphone distance cues

### 13.3 What Increases Likability

- **Pleasantness and trustworthiness** are the strongest predictors of voice likability
- **Pitch manipulation** affects trustworthiness perception
- **Prosodic variability** and "imperfect moments" increase naturalness
- **Breathing sounds and pauses** create embodied presence

### 13.4 Voice Naturalness Framework (2025)

A recent review in Trends in Cognitive Sciences (2025) proposes two distinct types of voice naturalness:
- **Deviation-based naturalness**: How far a voice deviates from typical human voice patterns
- **Human-likeness-based naturalness**: How closely a voice resembles a real human

These map to different processing pathways and have different acoustic correlates. The key insight is that naturalness is multidimensional -- it encompasses prosody, timing, variability, voice quality, and embodiment cues.

**Actionable for O-Formant**: Rather than pursuing maximum acoustic realism (which can backfire), focus on:
1. Consistent, non-deviant voice quality (avoid artifacts that sound "wrong")
2. Micro-variations (jitter, shimmer, vibrato irregularity) that signal organic production
3. Breathing/aspiration noise at appropriate moments
4. Avoid the "too perfect" trap -- synthesized voices sound more natural with imperfections

---

## 14. Vocal Tract Wall Effects

Research (Fleischer et al., 2015) demonstrates that vocal tract walls are NOT rigid:

- Wall impedance acts as distributed mass-damping system
- Normalized inertial component: 250 g/m^2 (>3 kHz) to 250,000 g/m^2 (~1.5-3 kHz)
- Shifts formant frequencies downward from rigid-wall predictions
- Increases formant bandwidths (especially F1)
- Effect is vowel-dependent and geometry-dependent

**Practical effect for synthesis**: The piriform fossae, transvelar coupling, and laryngeal wall vibration all make speech more acoustically realistic but a study (2021) found they can reduce perceived naturalness in articulatory synthesis by emphasizing low frequencies (muffled quality). A tenser voice source compensates.

**Actionable for O-Formant**: Model wall effects implicitly through slightly wider bandwidths (especially B1) and slightly lower formant frequencies than textbook rigid-tube predictions. The current bandwidth values are already in a reasonable range.

---

## 15. Priority Improvements for O-Formant

Based on this research, ranked by expected perceptual impact and implementation feasibility:

### Tier 1: High Impact, Moderate Effort
1. **Jitter and shimmer modeling** -- Per-cycle random perturbation of f0 and amplitude. Use 1/f noise, scale inversely with pitch/intensity. Most impactful single addition for naturalness.
2. **Dynamic Rd modulation** -- Vary vocal effort with pitch (higher pitch = slightly more pressed), velocity, and expression CC. Currently Rd is static per voice.
3. **Vibrato humanization** -- Add rate wobble, onset delay, depth modulation. Link to expression controllers.
4. **Formant bandwidth variation** -- Make B1 vary with vowel openness and breathiness. Wider B1 for open vowels and breathy voice.

### Tier 2: High Impact, Higher Effort
5. **Nasalization pole-zero pair** -- Single controllable nasal resonance parameter. Major addition for vocal character variety.
6. **Cascade formant option** -- Series resonator chain for vowel path (alongside existing parallel path for consonants). Produces more natural relative amplitudes.
7. **Formant transition modeling** -- Smooth interpolation between vowel targets with appropriate timing. Currently transitions are instantaneous when morph position changes.
8. **Spectral tilt control** -- Independent of Rd, add a tilt filter to shape the harmonic slope. Maps directly to H1-H2 perception.

### Tier 3: Specialized/Advanced
9. **Source-filter interaction approximation** -- Boost/attenuate harmonics near formant frequencies. Subtle but adds "life."
10. **Singer's formant mode** -- Allow F3-F5 clustering around 3 kHz for operatic singing style.
11. **Subglottal resonance zeros** -- Subtle spectral notches near 600 Hz and 1400 Hz.
12. **Consonant VOT and formant locus** -- More detailed consonant articulation model.
13. **Aspiration noise spectral shaping** -- Pitch-synchronous noise modulation (noise concentrated near glottal opening phase) rather than continuous white noise.

---

## Key References

### Foundational Papers
- Klatt, D.H. (1980). "Software for a cascade/parallel formant synthesizer." JASA, 67(3), 971-995.
- Klatt, D.H. & Klatt, L.C. (1990). "Analysis, synthesis and perception of voice quality variations among male and female talkers." JASA, 87(2), 820-857.
- Fant, G., Liljencrants, J., & Lin, Q. (1985). "A four-parameter model of glottal flow." STL-QPSR, 26(4), 1-13.
- Kelly, J.L. & Lochbaum, C.C. (1962). "Speech synthesis." Proc. 4th International Congress on Acoustics.

### Glottal Source
- Kreiman, J., Gerratt, B.R., & Antoanzas-Barroso, N. (2015). "Perceptual evaluation of voice source models." JASA.
- Lu, H.L. & Smith, J.O. (2000). "Glottal source modeling for singing voice synthesis." ICMC.
- Stochastic models of glottal pulses from the Rosenberg and Liljencrants-Fant models (2021). Computer Speech & Language.

### Source-Filter Coupling
- Titze, I.R. (2008). "Nonlinear source-filter coupling in phonation: Theory." JASA, 123(5), 2733-2749.
- Titze, I.R. (2008). "Nonlinear source-filter coupling in phonation: Vocal exercises." JASA, 123(4), 1902-1915.

### Vocal Tract Modeling
- Birkholz, P. (2013). "Modeling consonant-vowel coarticulation for articulatory speech synthesis." PLOS One, 8(4), e60603.
- Mullen, J. (2006). "Physical Modelling of the Vocal Tract with the 2D Digital Waveguide Mesh." PhD thesis, University of York.
- Cook, P. (1991). "Identification of control parameters in an articulatory vocal tract model." PhD thesis, Stanford/CCRMA.
- Fleischer, M. et al. (2015). "Formant frequencies and bandwidths of the vocal tract transfer function are affected by the mechanical impedance of the vocal tract wall." Biomechanics and Modeling in Mechanobiology.

### Vocoders and Singing Synthesis
- Morise, M., Yokomori, F., & Ozawa, K. (2016). "WORLD: A vocoder-based high-quality speech synthesis system for real-time applications." IEICE Trans. Info. & Sys., E99-D(7), 1877-1884.
- Yu, C.Y. & Fazekas, G. (2024). "GOLF: A singing voice synthesiser with glottal flow wavetables and LPC filters." Trans. ISMIR.

### Perception and Uncanny Valley
- Kuhne, K. et al. (2020). "The human takes it all: Humanlike synthesized voices are perceived as less eerie." Frontiers in Psychology.
- Koudenburg, N. et al. (2024). "Deviation from typical organic voices best explains a vocal uncanny valley." iScience.
- Understanding voice naturalness (2025). Trends in Cognitive Sciences.

### Nasalization
- Maeda, S. (1982). "The role of the sinus cavities in the production of nasal vowels."
- Styler, W. (2017). "On the acoustical features of vowel nasality in English and French." JASA.

### Articulatory Synthesis
- Birkholz, P. (2006). "Construction and control of a three-dimensional vocal tract model." ICASSP.
- VocalTrax (NeurIPS 2024 Workshop). Articulatory synthesis via optimization.
- SPARC (2024). Neural articulatory encoding-decoding framework.

### Consonant Synthesis
- Birkholz, P., Kroger, B.J., & Neuschaefer-Rube, C. (2013). "Modeling consonant-vowel coarticulation for articulatory speech synthesis." PLOS One.

### Piriform Fossae and Subglottal
- Effects of the piriform fossae, transvelar acoustic coupling, and laryngeal wall vibration on the naturalness of articulatory speech synthesis (2021). Speech Communication.
- Delvaux, V. & Howard, D. (2014). "A new method to explore the spectral impact of the piriform fossae on the singing voice." PLOS One.
