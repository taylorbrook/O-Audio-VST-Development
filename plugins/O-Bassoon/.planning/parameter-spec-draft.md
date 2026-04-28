# O-Bassoon Parameter Specification (Draft)

> Extracted from BRIEF.md. Full spec will be finalized during UI mockup phase.

## Parameters

### Vibrato

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| vibrato_rate | Vibrato Rate | 0.0–10.0 Hz | 5.0 | Float | Pitch LFO rate for vibrato |
| vibrato_depth | Vibrato Depth | 0–100 cents | 15 | Float | Pitch LFO amplitude |
| vibrato_onset | Vibrato Onset | 0–2000 ms | 400 | Float | Delay before vibrato fades in |

### Expression / Dynamics

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| breath | Breath / Dynamics | 0.0–1.0 | 0.7 | Float | Continuous loudness for swells (CC2 + velocity) |
| tone | Tone / Brightness | 0.0–1.0 | 0.5 | Float | Modal damping shaping dark↔bright timbre |
| attack_character | Attack Character | 0.0–1.0 | 0.0 | Float | Onset shape: 0=Soft pad, 1=Tongued articulation |

### Envelope

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| attack_time | Attack Time | 0–2000 ms | 300 | Float | Per-note onset envelope |
| release_time | Release Time | 0–3000 ms | 800 | Float | Per-note release envelope |

### Voicing / Output

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| voice_count | Voice Count | 1–16 | 8 | Int | Polyphony cap (oldest-note stealing) |
| output_gain | Output Gain | -24.0 to +6.0 dB | 0.0 | Float (dB) | Master output |

## MIDI / MPE / Note Expression Mapping

| Source | Target | Mapping |
|--------|--------|---------|
| Note number | Modal bank fundamental | Pitch via tuning engine, range C1–C6 |
| Velocity | Initial dynamic + attack character | Scales loudness on note-on; biases attack character |
| CC2 (Breath) | Breath / Dynamics | Primary continuous loudness |
| Aftertouch | Vibrato depth (TBD) | Optional modulation — confirm in Stage 0 |
| MPE channel pitch-bend | Per-note pitch | Standard MPE microtonality |
| VST3 Note Expression (pitch ID 0x00000003) | Per-note pitch | Dorico microtonal playback (O-Lyrica spike pattern) |

## Total Parameters: 10

## Notes

- Final parameter list to be locked during Stage 0 planning.
- Aftertouch → vibrato depth mapping is a Stage 0 decision (v1.0 stretch / v1.1 candidate per BRIEF).
- Attack Character is normalized 0–1 in DSP; UI may present as Soft↔Tongued enum or continuous slider (mockup decision).
