# O-Bowed - Requirements

---
version: 1.0.0
plugin: O-Bowed
created: 2026-04-04
lastUpdated: 2026-04-04
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 27
**Coverage:** must: 15 | should: 8 | nice: 4

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Bowed string synthesis via digital waveguide + nonlinear friction junction | must | complete | stage-2 |
| FUNC-02 | Tiered friction model: hyperbolic bow table (core), elasto-plastic bristle (enhanced), thermal rosin (quality) | must | complete | stage-2 |
| FUNC-03 | Morphable body resonator with Material (membrane-wood-metal-glass) and Size macros | must | complete | stage-2 |
| FUNC-04 | Configurable 1-4 active bowed strings | must | complete | stage-2 |
| FUNC-05 | Sympathetic string coupling (0-12 passive waveguide strings) | should | complete | stage-2 |
| FUNC-06 | Hybrid bow behavior: sustained while held, velocity/CC controls articulation | must | complete | stage-2 |
| FUNC-07 | Continuous impossible physics knobs (infinite sustain, reversed friction, sub-harmonics) | should | complete | stage-2 |
| FUNC-08 | Bow noise generator in signal chain | should | complete | stage-2 |
| FUNC-09 | Bridge filter with Brightness cutoff control | must | complete | stage-2 |
| FUNC-10 | Instrument presets (violin, cello, viola, bass, erhu, sarangi, nyckelharpa) | should | complete | stage-4 |
| FUNC-11 | Sound design presets (glass bow, metal drone, impossible strings, breath of strings) | nice | complete | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Digital waveguide string model with delay-line topology | must | complete | stage-2 |
| DSP-02 | Nonlinear friction junction with enhanced hyperbolic bow table | must | complete | stage-2 |
| DSP-03 | 2x internal oversampling for friction junction stability | must | complete | stage-2 |
| DSP-04 | Parallel biquad bank body resonator (6-10 sections) | must | complete | stage-2 |
| DSP-05 | Per-string stereo panning with Width parameter (0-200%) | should | complete | stage-2 |
| DSP-06 | Zero algorithmic latency (waveguide is causal) | must | complete | stage-2 |

### Tuning (TUNE)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| TUNE-01 | Scala/TUN file import for custom tuning systems | must | complete | stage-2 |
| TUNE-02 | MTS-ESP support for real-time retuning from host | must | complete | stage-2 |
| TUNE-03 | Per-string tuning offset with cent resolution (+/- 2400 cents) | must | complete | stage-2 |
| TUNE-04 | Adjustable reference pitch (A4 = 220-880 Hz, default 440) | should | complete | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | All parameters accessible and controllable from GUI | must | complete | stage-3 |
| UI-02 | Visual feedback for bow state (speed, pressure, position) | should | complete | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations in processBlock) | must | complete | stage-2 |
| PERF-02 | CPU per string (core tier) < 2% | should | pending | stage-2 |
| PERF-03 | CPU total (2 strings + body) < 6% | nice | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | complete | stage-2 |
| COMPAT-02 | Full MPE support — per-note pitch bend, pressure (Z), slide (Y) | must | complete | stage-2 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges | must | complete | stage-2 |
| QUAL-02 | Stable friction junction across full Schelleng diagram (bow speed x pressure) | nice | complete | stage-2 |
| QUAL-03 | Smooth parameter transitions without clicks or glitches | nice | complete | stage-2 |

## Acceptance Criteria Details

### FUNC-01: Bowed String Waveguide Synthesis

**Description:** Core synthesis engine using digital waveguide string model excited by nonlinear bow-string friction junction. Must produce recognizable bowed string tones.

**Acceptance Criteria:**
- [ ] Waveguide produces pitched output at correct MIDI frequency
- [ ] Friction junction produces audible stick-slip behavior
- [ ] Tone changes with bow speed, pressure, and position parameters

### FUNC-02: Tiered Friction Model

**Description:** Three-tier friction model where each tier adds fidelity. Core tier always active, enhanced and quality tiers add attack bite and sustained tone evolution.

**Acceptance Criteria:**
- [ ] Core (hyperbolic bow table) produces smooth bowed tone
- [ ] Enhanced (elasto-plastic) adds noticeable attack character
- [ ] Quality (thermal rosin) produces audible sustained tone evolution

### FUNC-03: Morphable Body Resonator

**Description:** Parallel biquad bank with Material and Size macro parameters that morph between distinct body types.

**Acceptance Criteria:**
- [ ] Material parameter audibly changes tonal character across full range
- [ ] Size parameter shifts resonant frequencies (small = higher, large = lower)
- [ ] Membrane, wood, metal, and glass positions are perceptually distinct

### FUNC-04: Multi-String Configuration

**Description:** User-configurable 1-4 active strings, each independently bowed.

**Acceptance Criteria:**
- [ ] Single string mode produces clean monophonic output
- [ ] Multiple strings produce independent pitches simultaneously
- [ ] String count changes without audio glitches

### FUNC-06: Hybrid Bow Behavior

**Description:** Bow sustained while MIDI note held, with velocity/CC controlling articulation style.

**Acceptance Criteria:**
- [ ] Note-on starts bowing, note-off releases with natural decay
- [ ] Velocity affects initial attack character
- [ ] CC modulation changes bow behavior in real-time

### FUNC-09: Bridge Filter

**Description:** Bridge filter with Brightness cutoff control shaping string output before body resonator.

**Acceptance Criteria:**
- [ ] Brightness parameter audibly controls high-frequency content
- [ ] Filter is real-time safe and produces no artifacts

### DSP-01: Digital Waveguide

**Description:** Delay-line based string model following Smith waveguide topology.

**Acceptance Criteria:**
- [ ] Correct pitch tracking across MIDI range (A0-C8)
- [ ] Fractional delay interpolation for accurate tuning
- [ ] Stable at all supported sample rates (44.1k-192k)

### DSP-02: Nonlinear Friction Junction

**Description:** Enhanced hyperbolic bow table computing reflected/transmitted waves at bow-string contact.

**Acceptance Criteria:**
- [ ] Newton-Raphson or equivalent solver converges within 2x oversampled loop
- [ ] Produces characteristic stick-slip waveform
- [ ] Stable across full bow speed/pressure parameter space

### DSP-03: 2x Oversampling

**Description:** Internal 2x oversampling specifically for friction junction computation.

**Acceptance Criteria:**
- [ ] Friction junction computed at 2x sample rate
- [ ] Anti-aliasing filter on downsample path
- [ ] No audible aliasing artifacts

### TUNE-01: Scala/TUN Import

**Description:** Load custom tuning systems from standard Scala (.scl) and TUN (.tun) file formats.

**Acceptance Criteria:**
- [ ] Correctly parses standard Scala files
- [ ] Correctly parses TUN files
- [ ] Applied tuning produces correct pitches (verified against reference)

### TUNE-02: MTS-ESP Support

**Description:** Real-time retuning via MTS-ESP protocol from external host applications.

**Acceptance Criteria:**
- [ ] Registers as MTS-ESP client
- [ ] Receives and applies real-time tuning changes
- [ ] Falls back gracefully when no MTS-ESP master present

### TUNE-03: Per-String Tuning

**Description:** Independent pitch offset per string with cent-level resolution.

**Acceptance Criteria:**
- [ ] Each string independently tunable +/- 2400 cents
- [ ] Cent-level precision maintained
- [ ] Tuning changes applied without audio glitches

### COMPAT-02: MPE Support

**Description:** Full MIDI Polyphonic Expression support for expressive per-note control.

**Acceptance Criteria:**
- [ ] Per-note pitch bend applied correctly
- [ ] Aftertouch/pressure (Z) maps to bow pressure
- [ ] Slide (Y) maps to bow position
- [ ] Works with MPE controllers (Linnstrument, Seaboard, Sensel)

### PERF-01: Real-Time Safe Processing

**Description:** No memory allocations, locks, or blocking operations in audio callback.

**Acceptance Criteria:**
- [ ] processBlock contains no new/delete/malloc/free
- [ ] No mutex locks in audio thread
- [ ] No file I/O in audio thread

### COMPAT-01: Pluginval Validation

**Description:** Plugin passes pluginval automated validation for both VST3 and AU formats.

**Acceptance Criteria:**
- [ ] Passes pluginval level 5 (strictest) for VST3
- [ ] Passes pluginval level 5 for AU
- [ ] No crashes or assertion failures during validation

### QUAL-01: No Audio Artifacts

**Description:** Clean audio output at normal parameter ranges without clicks, pops, or aliasing.

**Acceptance Criteria:**
- [ ] No clicks during parameter changes
- [ ] No DC offset in output
- [ ] No audible aliasing at standard sample rates

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-*, DSP-*, TUNE-*, PERF-01, COMPAT-02, QUAL-* |
| stage-3 | UI-* |
| stage-4 | all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Plucked string mode | Focus on bowed excitation for v1.0 | v1.1+ |
| Convolution body resonance | Biquad bank sufficient, convolution adds latency | v2.0+ |
| Multi-bow (multiple bows per string) | Complexity vs. payoff | v2.0+ |
| Built-in reverb/effects | Users have their own FX chains | v1.1+ |

---
*Generated from BRIEF.md on 2026-04-04*
