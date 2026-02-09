# Ouaricon Marimba - Parameter Specification (Draft)

## Overview

Minimal control set for a physically modeled marimba with native microtonal support.

## Parameters

### Sound Shaping

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| MALLET_HARDNESS | Mallet Hardness | Float | 0.0 - 1.0 | 0.5 | - | Soft (0) to hard (1) mallet character |
| BAR_MATERIAL | Bar Material | Float | 0.0 - 1.0 | 0.5 | - | Dark rosewood (0) to bright synthetic (1) |
| RESONANCE | Resonance | Float | 0.0 - 1.0 | 0.6 | - | Body coupling and sustain amount |

### Tuning

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| TUNING_MODE | Tuning Mode | Choice | 0-2 | 0 | - | 0=12-TET, 1=Scala, 2=MTS-ESP |
| REFERENCE_PITCH | Reference Pitch | Float | 400.0 - 480.0 | 440.0 | Hz | A4 reference frequency |

### Dynamics

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| VEL_CURVE | Velocity Curve | Float | 0.0 - 1.0 | 0.5 | - | Linear (0) to exponential (1) response |
| OUTPUT_GAIN | Output Gain | Float | -24.0 - 12.0 | 0.0 | dB | Master output level |

## Non-Automatable Settings

These are managed via UI but not exposed as DAW parameters:

| Setting | Description |
|---------|-------------|
| Scala File Path | Path to loaded .scl file |
| KBM File Path | Path to loaded .kbm file |
| IR Selection | Which body IR to use (if multiple provided) |

## Parameter Groups

1. **Sound** - MALLET_HARDNESS, BAR_MATERIAL, RESONANCE
2. **Tuning** - TUNING_MODE, REFERENCE_PITCH
3. **Output** - VEL_CURVE, OUTPUT_GAIN

## Notes

- Total automatable parameters: 7
- Tuning file paths are state (saved/restored) but not automatable
- MTS-ESP integration is automatic when master is present
- Velocity curve affects both brightness and level
