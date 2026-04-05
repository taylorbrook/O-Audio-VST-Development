# O-Bowed - Parameter Specification (Draft)

## Overview

Physical modeling bowed string synthesizer with tiered friction models, morphable body resonator, and microtonal support.

## Parameters

### Bow Controls

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| BOW_SPEED | Bow Speed | Float | 0.02 - 2.0 | 0.2 | m/s | Velocity of bow across string |
| BOW_PRESSURE | Bow Pressure | Float | 0.01 - 5.0 | 0.5 | N | Normal force of bow on string |
| BOW_POSITION | Bow Position | Float | 0.02 - 0.30 | 0.12 | - | Contact point (sul ponticello to sul tasto) |
| ROSIN | Rosin | Float | 0.0 - 1.0 | 0.5 | - | Friction curve shape (smooth to aggressive) |

### Body Controls

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| BODY_MATERIAL | Material | Float | 0.0 - 1.0 | 0.4 | - | Body morph: membrane <-> wood <-> metal <-> glass |
| BODY_SIZE | Size | Float | 0.0 - 1.0 | 0.5 | - | Body resonant frequency scaling (violin <-> bass) |
| BRIGHTNESS | Brightness | Float | 20.0 - 20000.0 | 8000.0 | Hz | Bridge filter cutoff |

### String Configuration

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| STRING_COUNT | String Count | Int | 1 - 4 | 1 | - | Number of active bowed strings |
| STRING_TUNING_1 | String 1 Tuning | Float | -2400.0 - 2400.0 | 0.0 | cents | Pitch offset for string 1 |
| STRING_TUNING_2 | String 2 Tuning | Float | -2400.0 - 2400.0 | 0.0 | cents | Pitch offset for string 2 |
| STRING_TUNING_3 | String 3 Tuning | Float | -2400.0 - 2400.0 | 0.0 | cents | Pitch offset for string 3 |
| STRING_TUNING_4 | String 4 Tuning | Float | -2400.0 - 2400.0 | 0.0 | cents | Pitch offset for string 4 |
| SYMPATHETIC_AMOUNT | Sympathetic Amount | Float | 0.0 - 1.0 | 0.0 | - | Coupling to passive sympathetic strings |
| SYMPATHETIC_COUNT | Sympathetic Strings | Int | 0 - 12 | 0 | - | Number of passive waveguide strings |

### Output

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| WIDTH | Stereo Width | Float | 0.0 - 2.0 | 1.0 | - | Stereo spread of multi-string output |
| OUTPUT_LEVEL | Output Level | Float | -60.0 - 12.0 | 0.0 | dB | Master output gain |

### Impossible Physics

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| INFINITE_SUSTAIN | Infinite Sustain | Float | 0.0 - 1.0 | 0.0 | - | Reduces damping toward zero |
| REVERSED_FRICTION | Reversed Friction | Float | 0.0 - 1.0 | 0.0 | - | Inverts friction curve |
| SUB_HARMONICS | Sub-Harmonics | Float | 0.0 - 1.0 | 0.0 | - | Sub-octave content via nonlinear feedback |

### Tuning

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| REFERENCE_PITCH | Reference Pitch | Float | 220.0 - 880.0 | 440.0 | Hz | A4 reference frequency |
| TUNING_SYSTEM | Tuning System | Choice | 0-2 | 2 | - | 0=Scala/TUN, 1=MTS-ESP, 2=12-TET |

## Non-Automatable Settings

| Setting | Description |
|---------|-------------|
| Scala File Path | Path to loaded .scl tuning file |
| KBM File Path | Path to loaded .kbm keyboard mapping file |
| Friction Model Tier | Core / Enhanced / Quality (CPU/quality tradeoff) |

## Parameter Groups

1. **Bow** - BOW_SPEED, BOW_PRESSURE, BOW_POSITION, ROSIN
2. **Body** - BODY_MATERIAL, BODY_SIZE, BRIGHTNESS
3. **Strings** - STRING_COUNT, STRING_TUNING_1-4, SYMPATHETIC_AMOUNT, SYMPATHETIC_COUNT
4. **Impossible** - INFINITE_SUSTAIN, REVERSED_FRICTION, SUB_HARMONICS
5. **Tuning** - REFERENCE_PITCH, TUNING_SYSTEM
6. **Output** - WIDTH, OUTPUT_LEVEL

## Notes

- Total automatable parameters: 22
- Per-string tuning offsets are separate parameters for DAW automation
- Friction model tier is a settings-level choice (not automatable)
- Tuning file paths are state (saved/restored) but not automatable
- MPE mapping: Pressure->BOW_PRESSURE, Slide->BOW_POSITION, Strike->velocity
