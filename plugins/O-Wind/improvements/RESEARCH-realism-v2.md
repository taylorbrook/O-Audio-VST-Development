# O-Wind Realism Improvements Research

**Date:** 2026-04-06
**Current Version:** 1.1.0
**Target:** v2.0.0 (MINOR — new parameters + behavioral changes)
**Scope:** Increase physical model realism through humanization, new parameters, spectral accuracy, and advanced modeling

---

## Executive Summary

Four parallel research streams investigated improvements to O-Wind's flute physical model. The findings converge on three tiers of improvements ranked by impact-per-effort. The current model's biggest realism gaps are: (1) sterile note attacks lacking the characteristic flute "chiff", (2) mechanical vibrato with no human variation, (3) uniform spectral envelope across registers, and (4) missing expressive articulations (flutter tongue, growl).

---

## Tier 1: High Impact, Low Complexity

These improvements deliver the biggest perceptual gains for minimal implementation effort. Each is independently valuable.

### 1.1 Attack Transient Modeling (CHIFF)

**Problem:** Current model uses a simple linear envelope ramp (5-30ms). Real flute attacks have four distinct phases:
1. Volume flux injection (0-2ms) — broadband noise burst as jet forms
2. Impulsive vortex shedding (2-5ms) — jet impacts labium for first time
3. Edge-tone regime (5-15ms) — inharmonic oscillations before bore coupling
4. Pipe-tone regime (>15ms) — bore locks jet oscillation, pitch settles

**Implementation:**
- Boost noise level by 3-6x during first 20-40ms of attack, then exponential decay to steady-state
- Add pitch overshoot: start bore delay 1-2% shorter than target, settle over 50-100ms via one-pole filter
- Scale both effects by MIDI velocity (hard attacks = more chiff)
- Optional: inject embouchure-cavity resonant noise (~1-2 kHz) during first 30ms

**New parameter:** `attackChiff` (0-1, default 0.5) — controls transient noise burst intensity
**CPU:** Near zero — envelope-gated noise, active only during first ~40ms per note
**Impact:** VERY HIGH — attack transients are the single most identifiable characteristic of real vs synthesized flute

### 1.2 Vibrato Humanization

**Problem:** Current vibrato is a pure sine LFO — the biggest "synth tell" on sustained notes.

**Implementation:**
- **Delayed onset:** Ramp vibrato depth from 0 to target over 200-500ms after note start
- **Rate drift:** Modulate LFO rate with slow noise (~0.5 Hz), +/- 0.5-1.0 Hz
- **Depth drift:** Independent slow noise modulation, +/- 20-30% of set depth
- **Shape asymmetry:** Replace `sin(phase)` with `sin(phase) + 0.1 * sin(2*phase)`
- **Per-cycle jitter:** Sample-and-hold noise at vibrato rate, 5-10% variation on rate and depth
- **Random initial phase:** Per-note random vibrato phase at noteOn

**New parameter:** `vibratoOnset` (0-1000ms, default 300ms) — delay before vibrato begins
**CPU:** Negligible — a few extra operations per sample in existing LFO
**Impact:** HIGH — transforms mechanical oscillation into organic human vibrato

### 1.3 Per-Note Randomization

**Problem:** Every note sounds identical given the same parameters — "machine gun" effect on repeated notes.

**Implementation:** At each `noteOn`, draw random offsets from per-voice RNG:

| Parameter | Random Range | Effect |
|-----------|-------------|--------|
| Attack time | +/- 20% of base | No two attacks identical |
| Noise burst amplitude | +/- 30% | Varied chiff intensity |
| Embouchure offset | +/- 0.5-1% of bore delay | Slight timbre shift at onset |
| Noise bandpass center | +/- 10% of Strouhal freq | Subtle breath color variation |
| Vibrato onset delay | +/- 50ms | Natural onset variation |
| Vibrato initial phase | 0 to 2π random | No phase-lock between notes |

**New parameter:** `humanize` (0-1, default 0.3) — master scale for all randomization amounts
**CPU:** Zero per-sample — random number generation only at noteOn
**Impact:** HIGH — eliminates machine-gun effect, essential for rapid passages

### 1.4 Breath Pressure Micro-Jitter

**Problem:** Sustained notes have frozen, static timbre. Real breath pressure fluctuates continuously.

**Implementation:**
- Generate white noise, filter through 1-pole lowpass at ~10 Hz (approximates pink/brownian character)
- Scale to 1-3% of current breath pressure, multiply onto `breathPressureParam`
- This creates slow, organic drift in amplitude and timbre

**New parameter:** Could be folded into `humanize` macro, or independent `breathStability` (0-1, default 0.8)
**CPU:** Negligible — one noise sample + one filter per sample
**Impact:** MEDIUM-HIGH — removes "frozen" quality of sustained notes

### 1.5 Pitch Micro-Variation

**Problem:** Perfectly tuned waveguide output sounds "locked to grid."

**Implementation:**
- Pink noise filtered to 0.1-5 Hz range, scaled to +/- 3-8 cents
- Applied as modulation to bore delay line length
- Independent of vibrato; correlate slightly with breath pressure (higher = sharper)
- Fold into `humanize` macro

**CPU:** Negligible — reuse breath jitter noise source
**Impact:** MEDIUM — subtle but important for sustained passages and chords

---

## Tier 2: High Impact, Medium Complexity

These require new DSP components but deliver significant feature additions.

### 2.1 Flutter Tongue

**Problem:** Essential flute articulation not available. Most-requested feature in wind synths.

**Implementation:**
- Amplitude modulation on jet excitation at 15-30 Hz
- `flutterTongue * sin(2π * flutterRate * t)` multiplied onto breath pressure in JetExciter
- Rate should have slight per-cycle randomization for naturalism
- Control via dedicated parameter or MIDI CC

**New parameters:** `flutterTongue` (0-1, default 0), `flutterRate` (15-30 Hz, default 22)
**CPU:** One extra LFO per voice — negligible
**Impact:** HIGH — immediately recognizable, frequently requested articulation

### 2.2 Growl

**Problem:** No vocal-fold coupling. Growling is a key expressive technique.

**Implementation:**
- Secondary low-frequency oscillator (70-120 Hz, sawtooth or pulse)
- Modulates jet reflection coefficient or summed into bore feedback path
- Creates characteristic roughness of vocal-fold coupling with air column

**New parameter:** `growl` (0-1, default 0)
**CPU:** One oscillator per voice — negligible
**Impact:** HIGH — expressive technique missing from current model

### 2.3 Material / Brightness Macro

**Problem:** Instrument presets switch between fixed coefficient sets. No way to continuously blend timbral character (metal vs wood vs bamboo).

**Implementation:** Single macro parameter that simultaneously controls:
- Bore loss filter cutoff (low = warm/wood, high = bright/metal)
- Noise bandpass center frequency (low = woody breath, high = metallic hiss)
- Radiation filter cutoff
- End reflection coefficient
- No new DSP — purely parameter-space remapping of existing filters

**New parameter:** `material` (0-1, default 0.5) where 0 = dark wood/bamboo, 1 = bright metal
**CPU:** Zero — parameter remapping only
**Impact:** MEDIUM-HIGH — transforms preset system from "pick instrument" to "design instrument"

### 2.4 Headjoint Formant Resonance

**Problem:** No formant-like spectral peak. Real flutes have a broad emphasis around 2-3 kHz from headjoint geometry.

**Implementation:**
- Add a parametric EQ (Q ~1.5, +3-6 dB) around 2.5 kHz in the radiation output path
- Center frequency varies per instrument preset:
  - Piccolo: ~4 kHz
  - Concert flute: ~2.5 kHz
  - Shakuhachi: ~1.8 kHz
  - Bansuri: ~2.0 kHz

**New parameter:** `formant` (0-1, default 0.5) — controls formant prominence (0 = bypass, 1 = +6dB)
**CPU:** One biquad per voice — minimal
**Impact:** MEDIUM-HIGH — addresses the biggest timbral gap vs SWAM

### 2.5 Register-Dependent Spectral Shaping

**Problem:** Bore loss filters apply uniform rolloff regardless of register. Low notes should have richer harmonics; high notes should be purer.

**Implementation:**
- Scale bore loss cutoff with MIDI note number: wider for low notes, narrower for high notes
- `adjustedCutoff = baseCutoff * (0.5 + noteNumber / 127.0)` (approximate)
- Also scale noise-to-tone ratio: more noise gain for low notes, less for high

**New parameter:** None needed — behavior change driven by MIDI note
**CPU:** Zero per sample — parameter calculation per block
**Impact:** MEDIUM — improves spectral accuracy across full range

### 2.6 Allpass Inharmonicity (Bore Detuning)

**Problem:** Bore model is perfectly harmonic. Real flutes have ~10-15 cents inharmonicity in upper partials from end corrections, tone holes, and bore perturbations.

**Implementation:**
- Insert 1-2 first-order allpass filters in bore feedback loop
- Allpass coefficient `a = 0.02-0.05` introduces frequency-dependent phase delay
- Upper harmonics shift by ~5-15 cents — matching measured flute inharmonicity
- Coefficient varies per preset: higher for shakuhachi/recorder (conical bore approximation)

**New parameter:** `inharmonicity` (0-1, default 0.3) or fold into preset coefficients
**CPU:** One multiply-add per allpass per sample — negligible
**Impact:** MEDIUM — dual-purpose: conical bore approximation + natural inharmonicity

---

## Tier 3: Medium Impact, Higher Complexity

### 3.1 Vocal Tract / Oral Cavity Coupling

A resonant cavity (1-2 poles) upstream of the jet exciter, coupled through nonlinear Bernoulli junction. Simulates tongue position and oral cavity shape — important for shakuhachi (meri/kari technique). New parameter: `oralCavity` (0-1). Complexity: medium-high due to nonlinear coupling stability concerns.

### 3.2 De la Cuadra Jet Offset

Adds jet offset variable to nonlinearity for asymmetric clipping — controls even/odd harmonic balance independently of embouchure. Different from the current symmetric tanh model. ~50-100 lines of DSP changes. New parameter: `jetOffset` or fold into embouchure.

### 3.3 Tonguing Articulation Types

Envelope shape presets for different tonguing: single (current), double (alternating attack strength), legato (overlapping envelopes). Could be a mode selector or articulation parameter.

### 3.4 Key Noise

Short filtered noise burst (1-3ms, 2-6 kHz) triggered on noteOn/noteOff at low amplitude. New parameter: `keyNoise` (0-1). Lower priority for flute than woodwinds.

### 3.5 Air Sound (Aeolian)

Direct path from turbulence noise to output, bypassing bore waveguide. Captures "all air, no tone" playing technique. Simple wet/dry mix.

### 3.6 Wire ToneHoleSystem

The 8-hole Keefe scattering system is implemented but not wired into the bore audio path. Wiring it enables: half-holing, cross-fingering timbres, and potentially multiphonics. Medium complexity — requires integrating scattering into the per-sample bore loop.

---

## Deferred / Not Recommended

| Feature | Reason |
|---------|--------|
| Wall vibration modeling | Scientific consensus: acoustically negligible (Coltman 1971) |
| Discrete vortex model | Very high CPU, marginal gain for flute geometries (Auvray 2014) |
| Full conical bore waveguide | Allpass approximation (2.6) covers 80% of the effect at 5% of the cost |
| Room/reverb | Post-processing, not PM-specific — users have DAW reverbs |
| Multiphonics | Wait for ToneHoleSystem wiring (3.6), should emerge naturally |

---

## Recommended Implementation Grouping

**Phase A (Humanization):** 1.1 + 1.2 + 1.3 + 1.4 + 1.5
- New params: `attackChiff`, `vibratoOnset`, `humanize`
- Touches: FluteSynthVoice, JetExciter
- No new DSP components

**Phase B (Articulations):** 2.1 + 2.2
- New params: `flutterTongue`, `flutterRate`, `growl`
- Touches: JetExciter (flutter), BoreWaveguide or FluteSynthVoice (growl)
- One new LFO per feature

**Phase C (Timbral):** 2.3 + 2.4 + 2.5 + 2.6
- New params: `material`, `formant`, `inharmonicity`
- Touches: BoreWaveguide (allpass, formant), FluteSynthVoice (material remapping)
- New: 1 biquad (formant) + 1-2 allpass (inharmonicity)

**Total new APVTS parameters: ~9-11**
- attackChiff, vibratoOnset, humanize
- flutterTongue, flutterRate, growl
- material, formant, inharmonicity

---

## Key References

- Verge (1995): Jet-drive model — current O-Wind foundation
- De la Cuadra (2005): Refined jet-drive with jet offset — Tier 3 extension
- Auvray et al. (2014 JASA): Regime change, attack transient phases — Tier 1 attack modeling
- Terrien et al. (2013): Multiphonic bifurcation analysis — deferred
- Fletcher (1975): Spectral analysis of flute by register — Tier 2 register shaping
- Coltman (1971): Wall material tests — confirms not needed
- Wolfe et al. (UNSW): Vocal tract coupling, impedance measurements — Tier 3
- Keefe (1990): Tone hole scattering junctions — already implemented, needs wiring
- SWAM Flutes parameter analysis — flutter, growl, formant priorities
