# Parameter Specification (Draft)

**Status:** Draft - Awaiting UI mockup for full specification
**Created:** 2026-07-23
**Source:** Derived from BRIEF.md parameter table (ideation)

This is a lightweight specification to enable parallel DSP research.
Full specification will be generated from finalized UI mockup.

## Parameters

### delayTime
- **Type:** Float
- **Range:** 50 to 2000 ms (log skew)
- **Default:** 500 ms
- **DSP Purpose:** Free-mode delay time — spacing of the reverse smear (active when syncMode = Free)

### syncMode
- **Type:** Choice
- **Choices:** Free, Sync
- **Default:** Sync
- **DSP Purpose:** Selects delay time source: free ms knob or host-tempo note division

### noteDivision
- **Type:** Choice
- **Choices:** 1/16, 1/16D, 1/16T, 1/8, 1/8D, 1/8T, 1/4, 1/4D, 1/4T, 1/2, 1/2D, 1/2T, 1/1
- **Default:** 1/4
- **DSP Purpose:** Tempo-synced delay time when syncMode = Sync; falls back to delayTime if host provides no BPM

### grainSize
- **Type:** Float
- **Range:** 50 to 500 ms (log skew)
- **Default:** 200 ms
- **DSP Purpose:** Length of each reversed grain — short = choppier, long = smoother smear

### density
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 60 %
- **DSP Purpose:** Grain overlap amount — higher = denser, more continuous wash

### feedback
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 40 %
- **DSP Purpose:** Regeneration of the reverse tail through the damped loop; must remain loop-stable at 100%

### lowCut
- **Type:** Float
- **Range:** 20 to 2000 Hz (log skew)
- **Default:** 100 Hz
- **DSP Purpose:** Low-cut filter inside the feedback loop — thins repeats over successive generations

### highCut
- **Type:** Float
- **Range:** 500 to 20000 Hz (log skew)
- **Default:** 8000 Hz
- **DSP Purpose:** High-cut filter inside the feedback loop — darkens repeats over successive generations

### width
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 60 %
- **DSP Purpose:** Stereo spread of grain placement across the field

### mix
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 35 %
- **DSP Purpose:** Dry/wet balance (equal-power crossfade)

## Next Steps

- [ ] Complete UI mockup workflow (/start → option 3)
- [ ] Finalize design and generate full parameter-spec.md
- [ ] Validate consistency between draft and final spec
