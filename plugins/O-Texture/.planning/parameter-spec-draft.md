# O-Texture - Parameter Specification (Draft)

## Overview

Neural texture synthesizer with custom 1D CNN VAE. Dual mode (Generate/Transform) with XY pad latent space navigation and evolving texture modulation.

## Parameters

### Mode & Source

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| SOURCE | Source | Choice | 0-5 | 0 | - | 0=Rain, 1=Metal, 2=Wind, 3=Crowd, 4=Synth, 5=Organic |
| MODE | Mode | Choice | 0-1 | 0 | - | 0=Generate (no input), 1=Transform (processes input) |

### Latent Space Navigation

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| X | X | Float | 0.0 - 1.0 | 0.5 | - | Latent space X position (most active dimension) |
| Y | Y | Float | 0.0 - 1.0 | 0.5 | - | Latent space Y position (second most active dimension) |
| CHARACTER_A | Character A | Float | 0.0 - 1.0 | 0.5 | - | Third latent dimension -- secondary timbral axis |
| CHARACTER_B | Character B | Float | 0.0 - 1.0 | 0.5 | - | Fourth latent dimension -- secondary timbral axis |
| EVOLVE | Evolve | Float | 0.0 - 1.0 | 0.3 | - | Rate of random walk through remaining latent dimensions |
| FREEZE | Freeze | Bool | Off/On | Off | - | Hold current latent position (stops evolution) |

### Output

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| BRIGHTNESS | Brightness | Float | -1.0 - 1.0 | 0.0 | - | Post-processing tilt EQ (-100% to +100%) |
| MIX | Mix | Float | 0.0 - 1.0 | 1.0 | - | Dry/wet blend (relevant in Transform mode) |

## Parameter Groups

1. **Source** - SOURCE, MODE
2. **Latent** - X, Y, CHARACTER_A, CHARACTER_B, EVOLVE, FREEZE
3. **Output** - BRIGHTNESS, MIX

## Notes

- Total automatable parameters: 10
- XY pad controls X and Y simultaneously via UI interaction
- Evolve modulates remaining 28 latent dimensions (of 32 total) via random walk
- Freeze stops all latent evolution but allows manual X/Y/Character changes
- Mix only audible in Transform mode (Generate mode is 100% wet by design)
- Source switching loads different ONNX model -- may cause brief crossfade gap
