# Parameter Specification (Draft)

**Status:** Draft - Awaiting UI mockup for full specification
**Created:** 2025-01-18
**Source:** Quick capture during ideation

This is a lightweight specification to enable parallel DSP research.
Full specification will be generated from finalized UI mockup.

## Parameters

### delayTime
- **Type:** Float
- **Range:** 50 to 2000 ms
- **Default:** 500
- **DSP Purpose:** Time between grain triggers. When tempo sync is enabled, snaps to note divisions.

### grainSize
- **Type:** Float
- **Range:** 5 to 500 ms
- **Default:** 100
- **DSP Purpose:** Length of each grain window. Affects texture density and character.

### feedback
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 30
- **DSP Purpose:** Amount of processed output fed back into the grain buffer for repeating delays.

### chaos
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 25
- **DSP Purpose:** Master randomization amount. Scales randomization of grain position, pitch (octaves/fifths), pan, and density.

### character
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 50
- **DSP Purpose:** Morphs between glitchy (0% - sparse grains, short crossfades) and smooth/angelic (100% - dense grains, long crossfades).

### mix
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 50
- **DSP Purpose:** Dry/wet balance between original signal and processed granular output.

### tempoSync
- **Type:** Bool
- **Default:** true
- **DSP Purpose:** When enabled, delay time snaps to host tempo note divisions (1/16, 1/8, 1/4, 1/2, 1).

## Technical Notes

- Pitch randomization constrained to octaves (+/- 12 semitones) and fifths (+/- 7 semitones)
- Mono input, stereo output with randomized pan per grain
- Grain envelope crossfade time controlled by character parameter
- Window functions (Hann or Tukey) for smooth grain envelopes

## Next Steps

- [ ] Complete UI mockup workflow (/dream AngelGrain → option 3)
- [ ] Finalize design and generate full parameter-spec.md
- [ ] Validate consistency between draft and final spec
