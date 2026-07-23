# O-ReverseDelay - Creative Brief

## Overview

**Type:** Effect
**Core Concept:** Granular reverse delay — overlapping reversed grains create smooth, washy backwards echoes for ambient swells and pads.
**Status:** 💡 Ideated
**Created:** 2026-07-23

## Vision

O-ReverseDelay is an ambient-focused reverse delay built on a granular engine. Instead of the classic chunked reverse-repeat (which produces hard, rhythmically rigid backwards blocks), the wet signal is assembled from overlapping reversed grains, yielding a continuous reverse *smear* — swells that bloom backwards into the mix rather than snapping in.

Delay time locks to host tempo via note divisions or runs free in milliseconds, so the swells can sit rhythmically in a track or float freely over it. The feedback path carries damping filters (low-cut and high-cut inside the loop) so regenerating tails darken and thin naturally, the way ambient washes decay in a real space. The design target is long, evolving reverse tails for atmospheric texture — pads, guitar swells, vocal ambience — with feedback high enough to approach self-sustaining washes without runaway.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| delayTime | 50–2000 ms (log skew) | 500 ms | Free-mode delay time — spacing of the reverse smear |
| syncMode | Free / Sync | Sync | Delay time source: free ms knob or host-tempo note division |
| noteDivision | 1/16 … 1/1 (incl. dotted, triplet) | 1/4 | Tempo-synced delay time when syncMode = Sync |
| grainSize | 50–500 ms (log skew) | 200 ms | Length of each reversed grain — short = choppier, long = smoother smear |
| density | 0–100% | 60% | Grain overlap amount — higher = denser, more continuous wash |
| feedback | 0–100% | 40% | Regeneration of the reverse tail through the damped loop |
| lowCut | 20–2000 Hz (log skew) | 100 Hz | Low-cut inside the feedback loop — thins repeats over time |
| highCut | 500–20000 Hz (log skew) | 8000 Hz | High-cut inside the feedback loop — darkens repeats over time |
| width | 0–100% | 60% | Stereo spread of grain placement |
| mix | 0–100% | 35% | Dry/wet balance (equal-power) |

## UI Concept

*(Not specified during ideation — to be designed in the UI mockup phase.)*

## Use Cases

- Ambient swells and pads: long reverse tails blooming under sustained material
- Guitar/keys volume-swell-style textures without playing technique
- Vocal ambience: reversed pre-echo washes behind a lead
- Near-self-oscillating wash beds at high feedback with heavy damping

## Inspirations

- Granular smear character (vs. classic chunked reverse of Boss DD-7 / RE-20 reverse modes)
- Ambient delay/texture pedals and plugins oriented toward swells rather than rhythmic repeats

## Technical Notes

- Granular engine with reversed grain playback over a circular capture buffer; overlapping windowed grains (Hann or similar) for click-free smear. Prior art in-suite: O-simpleGrain, O-GrainScatter, O-Freeze.
- Feedback damping filters must use RT-safe coefficient updates (`ArrayCoefficients`, assign in place — no `Coefficients::makeXXX` on the audio thread).
- Time and frequency parameters need log/skewed `NormalisableRange`; factory presets must be authored in engineering units + `convertTo0to1` (skew pitfall pattern).
- Tempo sync reads host BPM from `AudioPlayHead`; fall back to free time when no tempo is available.
- Feedback at 100% should be loop-stable with damping engaged (soft-clip or energy cap in loop), not runaway.
- Candidate for OuariconPresetManager module integration.

## Next Steps

- [ ] Create UI mockup (`/start O-ReverseDelay` → option 3)
- [ ] Start implementation (`/implement O-ReverseDelay`)
