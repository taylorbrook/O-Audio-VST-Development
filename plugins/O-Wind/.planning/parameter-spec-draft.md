# O-Wind Parameter Specification (Draft)

> Extracted from BRIEF.md. Full spec will be finalized during UI mockup phase.

## Parameters

### Breath/Excitation

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| breath_pressure | Breath Pressure | 0.0–1.0 | 0.5 | Float | Jet velocity (nonlinear: pressure^1.5 curve). CC2/Breath, Aftertouch |
| embouchure | Embouchure | 0.0–1.0 | 0.5 | Float | Jet delay ratio (0.3–0.6 of bore). Controls register/overblowing. CC74/MPE Y |
| breath_noise | Breath Noise | 0.0–1.0 | 0.15 | Float | Turbulence noise gain. Scaled by jet velocity squared |
| tone_color | Tone Color | 0.0–1.0 | 0.5 | Float | Bore reflection filter cutoff (1000–12000 Hz log). Brightness control |

### Resonator

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| air_column | Air Column | 0.0–1.0 | 0.5 | Float | Viscothermal loss amount. High = warmer (more HF loss) |
| jet_reflection | Jet Reflection | -1.0–1.0 | 0.5 | Float | Feedback coefficient from bore to jet |
| end_reflection | End Reflection | -1.0–1.0 | 0.5 | Float | Bore end reflection coefficient. Controls sustain character |

### Expression

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| vibrato_rate | Vibrato Rate | 2.0–8.0 | 5.0 | Float | Pressure vibrato rate in Hz |
| vibrato_depth | Vibrato Depth | 0.0–1.0 | 0.3 | Float | Modulation depth on breath pressure |

### Output

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| width | Width | 0.0–2.0 | 1.0 | Float | Stereo decorrelation |
| output_level | Output Level | -60.0–12.0 | 0.0 | Float (dB) | Master gain |

### Impossible Physics

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| infinite_sustain | Infinite Sustain | 0.0–1.0 | 0.0 | Float | Reduces bore losses toward zero |
| reversed_jet | Reversed Jet | 0.0–1.0 | 0.0 | Float | Inverts jet nonlinearity curve |
| sub_harmonics | Sub-Harmonics | 0.0–1.0 | 0.0 | Float | Sub-octave content via nonlinear feedback |

## MIDI/MPE Mapping

| Source | Target | Mapping |
|--------|--------|---------|
| Note number | Bore delay length | Pitch via tuning engine |
| Velocity | Breath attack ramp time | 127=5ms sharp, 1=30ms gentle |
| Aftertouch/CC2 | Breath pressure | Primary dynamics |
| MPE Y (CC74/Slide) | Embouchure | Continuous timbre control |
| Pitch bend | Bore delay modulation | Smooth portamento |
| CC1 (Mod) | Vibrato depth | Standard mapping |
| CC11 (Expression) | Output level | Standard dynamics |

## Total Parameters: 13
