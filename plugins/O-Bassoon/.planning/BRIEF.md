# O-Bassoon - Creative Brief

## Overview

**Type:** Synth (Physical Model Bassoon)
**Core Concept:** A simple modal-synthesis bassoon optimized for sustained microtonal long tones with full Ouaricon-family per-note pitch control.
**Status:** Ideated
**Created:** 2026-04-27

## Vision

O-Bassoon is a clean, focused contribution to the Ouaricon physical-model wind family. The instrument prioritizes long sustained tones over articulated phrases — think held drones, swelled crescendos, slow microtonal glides, and chord stacks for ambient/orchestral textures.

The design deliberately avoids reusing any code or model from O-Reed (currently broken). Instead, it uses **modal synthesis** — a sum of damped resonant modes capturing the bassoon's spectral signature — which is simpler to implement than a full reed-excitation waveguide and excellent for stable, controllable long tones.

Microtonal capability is on par with the rest of the Ouaricon family: VST3 Note Expression for Dorico (already-validated spike pattern from O-Lyrica) plus MPE channel pitch-bend for DAW use.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Vibrato Rate | 0-10 Hz | 5.0 | Pitch LFO rate for vibrato |
| Vibrato Depth | 0-100 cents | 15 | Pitch LFO amplitude |
| Vibrato Onset | 0-2000 ms | 400 | Delay before vibrato fades in |
| Breath / Dynamics | 0-1 (CC2 + velocity) | 0.7 | Continuous loudness for swells |
| Tone / Brightness | 0-1 | 0.5 | Modal damping shaping dark↔bright timbre |
| Attack Character | Soft ↔ Tongued | Soft | Onset shape from pad-like to articulated |
| Attack Time | 0-2000 ms | 300 | Per-note onset envelope |
| Release Time | 0-3000 ms | 800 | Per-note release envelope |
| Voice Count | 1-16 | 8 | Polyphony cap |
| Output Gain | -24 to +6 dB | 0 | Master output |

*Final parameter list to be locked during Stage 0 planning.*

## UI Concept

To be designed in a separate UI mockup pass (`/start O-Bassoon` → option 3). UI inherits from Ouaricon family visual language; specifics deferred to mockup phase.

## Use Cases

- Sustained bassoon long tones for ambient/orchestral pads
- Microtonal drone work (per-note pitch via Note Expression / MPE)
- Slow chord stacks (8-voice polyphony) for rich harmonic textures
- Dorico playback for contemporary scores using non-12-TET tunings
- Crescendo/decrescendo articulations driven by CC2 breath control

## Inspirations

- **O-Lyrica** — validated VST3 Note Expression spike pattern for Dorico microtonality
- **O-Wind** — Ouaricon physical-model woodwind UX precedent (flute)
- **O-Bowed / O-Contrabass** — Ouaricon physical-model sustained-instrument precedent
- Real bassoon timbre, especially the lower register's woody resonance

## Technical Notes

- **DSP approach:** Modal synthesis — bank of damped resonators (parallel biquads or 2nd-order modal filters) tuned to bassoon's harmonic spectrum. Excitation is a soft impulse + low-level filtered noise; no reed self-oscillation model.
- **Range:** Extended C1-C6 (beyond the real bassoon's Bb1-Eb5) for unusual-register drones.
- **Polyphony:** 8-16 voices (default 8). Voice manager with oldest-note stealing.
- **Microtonal pitch:** VST3 Note Expression (per-note pitch ID 0x00000003) + MPE channel pitch-bend. Match the O-Lyrica spike pattern documented in `spike-findings-VST-development` skill.
- **Expression mapping:** CC2 → Breath; velocity → initial dynamic + attack character; aftertouch → optional vibrato depth modulation (TBD in Stage 0).
- **Real-time safety:** No allocations in `processBlock`; all mode banks pre-allocated per voice in `prepareToPlay`.
- **Explicit non-goals for v1.0:** No reed self-oscillation modeling, no key-click sample layer, no preset browser at v1.0 (deferred to v1.1).

## Next Steps

- [ ] Stage 0 planning (`/plan O-Bassoon`) — research modal synthesis bassoon spectra, finalize architecture
- [ ] UI mockup (`/start O-Bassoon` → option 3) — can run in parallel with Stage 0
- [ ] Implementation (`/implement O-Bassoon`) — after Stage 0 plan locked
