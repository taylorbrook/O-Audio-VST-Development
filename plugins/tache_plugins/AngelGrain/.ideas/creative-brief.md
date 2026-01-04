# AngelGrain - Creative Brief

## Overview

**Type:** Effect
**Core Concept:** Granular delay with musical randomization and morphable character from glitchy to ethereal
**Status:** Ideated
**Created:** 2025-01-18

## Vision

AngelGrain is a granular delay that balances chaos and beauty. At its core, it captures incoming audio into a grain buffer and scatters grains across time and stereo space with controllable randomization. The key tension is between "glitchy" (sparse, rhythmic, unpredictable grains) and "smooth/angelic" (dense, overlapping grains with long crossfades that blur into continuous pad textures).

Pitch randomization is constrained to octaves and fifths, keeping even chaotic settings harmonically coherent—celestial rather than atonal. A single "chaos" knob scales all randomization (position, pitch, pan, density), making it easy to dial in exactly the right amount of unpredictability.

Inspired by Mutable Instruments Clouds/Beads, but focused on the delay paradigm with traditional feedback and tempo-syncable delay times.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Delay Time | 50ms - 2s | 500ms | Time between grain triggers, tempo-syncable |
| Tempo Sync | On/Off | Off | Toggle between free (ms) and note divisions |
| Grain Size | 5ms - 500ms | 100ms | Length of each grain window |
| Feedback | 0% - 100% | 30% | Amount of output fed back into grain buffer |
| Chaos | 0% - 100% | 25% | Master randomization amount (scales position, pitch, pan, density) |
| Character | 0% - 100% | 50% | Morphs from glitchy (sparse, short crossfades) to smooth (dense, long crossfades) |
| Mix | 0% - 100% | 50% | Dry/wet balance |

## UI Concept

**Layout:** Central character knob as focal point, with chaos and grain parameters surrounding it
**Visual Style:** Ethereal, luminous—suggest halos, soft glows, particle clouds
**Key Elements:** Visual feedback showing grain activity/density, stereo field visualization

## Use Cases

- Transforming pads and drones into evolving textural soundscapes
- Adding ethereal, harmonically-rich ambience to vocals and melodies
- Creating glitchy rhythmic variations from sustained sounds
- Building dense, shimmering reverb-like tails with musical pitch content

## Inspirations

- Mutable Instruments Clouds/Beads (granular approach, freeze aesthetic)
- The harmonic series (octaves and fifths as "pure" intervals)
- Choral/angelic music (dense overlapping voices, cathedral reverbs)

## Technical Notes

- Pitch shifting to octaves (+/- 12 semitones) and fifths (+/- 7 semitones) only
- Mono input, stereo output with randomized pan per grain
- Grain envelope crossfade time controlled by Character parameter
- Grain density (grains per second) also modulated by Character and Chaos
- Consider window functions (Hann, Tukey) for smooth grain envelopes

## Next Steps

- [ ] Create UI mockup (`/dream AngelGrain` → option 3)
- [ ] Start implementation (`/implement AngelGrain`)
