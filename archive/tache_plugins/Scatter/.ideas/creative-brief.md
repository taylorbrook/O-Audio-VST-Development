# Scatter - Creative Brief
## Overview

**Type:** Effect
**Core Concept:** Granular reversed delay with beautiful stuttering grains, randomized pitch (quantized to musical scales), and randomized stereo placement
**Status:** 💡 Ideated
**Created:** 2025-11-12

## Vision

Scatter is a granular delay effect that transforms incoming audio into evolving, textural ambience through reversed playback, pitch-shifting grains, and randomized spatial placement. Each grain is a fragment of the delayed signal played backwards, with its pitch quantized to a musical scale and its position randomized across the stereo field. The result is a shimmering, stuttering delay that creates ambient soundscapes while maintaining musical coherence through scale quantization.

The plugin draws inspiration from granular processors like GrainScanner and Portal, ambient effects like Cosmos and Shimmer, and reverse delays like Backmask and H-Delay. It's designed for textural ambience—creating evolving soundscapes and atmospheric beds that transform source material into abstract, musical textures.

## UI Concept

**Layout:** Centered cluster arrangement with controls surrounding a central particle field visualization

**Visual Style:** Cream, spacey, textured skeuomorphic design with warm ethereal mood

**Key Elements:**
- Central 200×200px particle field visualization (animated grain cloud showing stereo/time/pitch positions)
- 7 large knobs (90px diameter) with 3D shadows and gradients arranged in cluster around particle field
- 2 combo boxes (Scale, Root Note) in header
- Textured background with grain/noise overlay
- Cosmic glow effects on particle visualization

**Color Scheme:**
- Warm cream base (#f5f0e8) with sand textures (#e8dfd0)
- Warm gold accents (#b8956f) for highlights
- Soft gold particle glow (#ffd89b) with brighter centers (#ffb347)
- Warm dark brown text (#5a4a3a)

**Dimensions:** 550×600px (non-resizable)
## Use Cases

- Creating evolving soundscapes and atmospheric beds from melodic loops or pads
- Transforming percussion into stuttering, pitched textures
- Adding ambient depth to vocals by creating quantized harmonic delays
- Building textural layers that evolve over time through feedback and randomization
- Sound design for film/games requiring abstract, musical atmospheres

## Inspirations

- **Granular:** GrainScanner, Portal, Granite, Iris (grain processing techniques)
- **Ambient:** Cosmos, Shimmer, CloudSeed (textural delay effects)
- **Reverse:** Backmask, H-Delay reverse mode, EchoBoy (reverse delay algorithms)

## Technical Notes

**DSP Considerations:**
- Circular delay buffer with tempo-sync support (linked to DAW tempo)
- Grain scheduler with overlap-based density control
- Per-grain reverse playback with windowing (Hann/Hamming to avoid clicks)
- Pitch-shifting algorithm for grains (time-domain stretching or frequency-domain shifting)
- Scale quantization system (±7 semitone range mapped to selected scale/root)
- Per-grain random pan position generator
- Feedback loop with traditional delay feedback architecture
- Each grain plays with random forward/reverse selection

**Implementation Strategy:**
- Grain engine with voice management (polyphonic grain playback)
- Windowing to prevent clicks on grain start/end
- Efficient pitch-shifting (spectral or time-domain)
- Real-time tempo sync (recalculate delay time on tempo changes)

## Next Steps

- [ ] Complete mockup finalization
- [ ] Run /implement to begin Stage 1 (Foundation)
- [ ] Proceed through workflow stages
- [ ] Test and validate plugin
