# Scatter Notes

## Status
- **Current Status:** 💡 Ideated
- **Version:** N/A
- **Type:** Audio Effect (Delay)

## Lifecycle Timeline

- **2025-11-12:** Creative brief completed

## Known Issues

None

## Additional Notes

**Description:**
Granular reversed delay with beautiful stuttering grains, randomized pitch (quantized to musical scales), and randomized stereo placement. Creates evolving textural ambience with musical coherence.

**Parameters (9 total):**
- Delay Time: 100ms-2s (synced), default 500ms (buffer length, tempo-synced note values)
- Grain Size: 5-500ms, default 100ms (individual grain length)
- Grain Density: 0-100%, default 50% (overlap amount between grains)
- Pitch Random: 0-100%, default 30% (pitch randomization amount, ±7 semitones max)
- Scale: Chromatic/Major/Minor/etc, default Chromatic (quantization scale)
- Root Note: C-B, default C (scale root)
- Pan Random: 0-100%, default 75% (stereo randomization amount)
- Feedback: 0-100%, default 30% (traditional delay feedback)
- Mix: 0-100%, default 50% (dry/wet blend)

**DSP:** Circular delay buffer with tempo sync. Grain scheduler with overlap-based density control. Per-grain reverse playback with windowing (Hann/Hamming). Pitch-shifting per grain with scale quantization (±7 semitone range). Random pan position generator. Each grain randomly forward or reverse.

**GUI:** Single-page interface with grain cloud visualization (shows position in time/stereo/pitch space). Tempo sync indicator. Scale/root note selector. Visual feedback for density and randomization.

**Use Cases:**
- Creating evolving soundscapes and atmospheric beds
- Textural ambience for melodic loops or pads
- Transforming percussion into stuttering, pitched textures
- Adding ambient depth with quantized harmonic delays
- Sound design for film/games requiring abstract, musical atmospheres

**Inspirations:**
- Granular: GrainScanner, Portal, Granite, Iris
- Ambient: Cosmos, Shimmer, CloudSeed
- Reverse delays: Backmask, H-Delay reverse mode, EchoBoy
