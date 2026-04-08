# O-Reed - Parameter Specification (Draft)

## Overview

Physical modeling reed wind instrument synthesizer with Guillemain confined jet parameter, parameterized bore profile, tone hole lattice, and MPE/breath controller support.

## Parameters

### Primary Controls (Tier 1)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| BREATH_PRESSURE | Breath Pressure | Float | 0.0 - 1.0 | 0.5 | - | Mouth pressure (p_mouth) — main dynamics |
| EMBOUCHURE | Embouchure / Bite | Float | 0.0 - 1.0 | 0.4 | - | Lip force — brightness, pitch bending |
| REED_HARDNESS | Reed Hardness | Float | 0.0 - 1.0 | 0.5 | - | Reed stiffness (k_r) — attack character, brightness |
| BORE_CHARACTER | Bore Character | Float | 0.0 - 1.0 | 0.0 | - | Bore taper — 0=cylindrical to 1=full cone |
| INSTRUMENT_PRESET | Instrument Morph | Choice | 0 - 20 | 0 | - | Macro crossfading full parameter sets between presets |

### Secondary Controls (Tier 2)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| REED_OPENING | Reed Opening | Float | 0.0 - 1.0 | 0.4 | - | Rest opening (H) — ease of onset, dynamic range |
| BELL_SIZE | Bell Size | Float | 0.0 - 1.0 | 0.5 | - | Bell flare — projection, high-frequency radiation |
| AIR_NOISE | Air Noise | Float | 0.0 - 1.0 | 0.15 | - | Breath noise mix — breathiness |
| DOUBLE_REED | Double Reed Amount | Float | 0.0 - 1.0 | 0.0 | - | Psi confinement — single to double reed character |
| BORE_DIAMETER | Bore Diameter | Float | 0.0 - 1.0 | 0.5 | - | Viscothermal loss and impedance |

### Advanced / Sound Design (Tier 3)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| REED_MASS | Reed Mass | Float | 0.0 - 1.0 | 0.3 | - | mu_r — reed resonance effects, transient character |
| REED_DAMPING | Reed Damping | Float | 0.0 - 1.0 | 0.5 | - | g_r — reed ring vs. muted bore-driven tone |
| MOUTHPIECE_VOL | Mouthpiece Volume | Float | 0.0 - 1.0 | 0.3 | - | Helmholtz resonance — pitch correction |
| TONE_HOLE_CUTOFF | Tone Hole Cutoff | Float | 200.0 - 8000.0 | 1500.0 | Hz | Spectral envelope from open tone holes |
| REGISTER_HOLE | Register Hole | Float | 0.0 - 1.0 | 0.0 | - | Overblowing control |
| BORE_LENGTH | Bore Length | Float | 0.0 - 1.0 | 0.5 | - | Effective tube length — register density |
| BORE_PROFILE | Bore Profile | Choice | 0 - 1 | 0 | - | Simple (single taper) vs Multi-segment |

### Expressive Controls

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| VIBRATO_DEPTH | Vibrato Depth | Float | 0.0 - 1.0 | 0.0 | - | LFO modulation depth |
| VIBRATO_RATE | Vibrato Rate | Float | 1.0 - 10.0 | 5.0 | Hz | LFO frequency |
| VIBRATO_SOURCE | Vibrato Source | Choice | 0 - 2 | 0 | - | Lip / Breath / Throat modulation target |
| GROWL_AMOUNT | Growl Amount | Float | 0.0 - 1.0 | 0.0 | - | Vocal fold coupling strength |
| FLUTTER_TONGUE | Flutter Tongue | Float | 0.0 - 1.0 | 0.0 | - | ~25 Hz pressure modulation |
| SUBTONE | Subtone | Float | 0.0 - 1.0 | 0.0 | - | Minimum reed opening + high lip damping |
| ATTACK_CHIFF | Attack Chiff | Float | 0.0 - 1.0 | 0.3 | - | Pressure overshoot at note onset |

### Impossible Physics (Sound Design)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| INFINITE_SUSTAIN | Infinite Sustain | Float | 0.0 - 1.0 | 0.0 | - | Reduces bore losses toward zero |
| REVERSE_BORE | Reverse Bore | Float | 0.0 - 1.0 | 0.0 | - | Negative taper (hichiriki-like, extended) |
| DUAL_BORE | Dual Bore | Bool | false - true | false | - | Second parallel waveguide (arghul/launeddas drone) |
| DRONE_PITCH | Drone Pitch | Float | -24.0 - 24.0 | 0.0 | semitones | Pitch offset for second bore |
| FEEDBACK_PATH | Feedback Path | Float | 0.0 - 1.0 | 0.0 | - | Cross-modulation between reed and bore |

### Tuning

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| REFERENCE_PITCH | Reference Pitch | Float | 220.0 - 880.0 | 440.0 | Hz | A4 reference frequency |
| TUNING_SYSTEM | Tuning System | Choice | 0 - 2 | 2 | - | Scala/TUN / MTS-ESP / 12TET |

### Voice Configuration

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| POLY_MODE | Polyphony Mode | Choice | 0 - 1 | 0 | - | Monophonic (default) / Polyphonic |
| MAX_VOICES | Max Voices | Int | 1 - 16 | 8 | - | Maximum polyphony (poly mode only) |
| OVERSAMPLING | Oversampling | Choice | 0 - 1 | 0 | - | 2x (default) / 4x (mono quality) |

### Output

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| OUTPUT_GAIN | Output Gain | Float | -60.0 - 12.0 | 0.0 | dB | Master output level |

## Parameter Count Summary

- Tier 1 (Primary): 5
- Tier 2 (Secondary): 5
- Tier 3 (Advanced): 7
- Expressive: 7
- Sound Design: 5
- Tuning: 2
- Voice Config: 3
- Output: 1
- **Total: 35**
