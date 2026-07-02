# Parameter Specification

**Status:** Complete
**Created:** 2025-01-18
**Source:** Converted from draft (no UI mockup)

## Parameters

### delayTime
- **ID:** delayTime
- **Type:** Float
- **Range:** 50.0 to 2000.0
- **Default:** 500.0
- **Unit:** ms
- **Skew:** 0.5 (logarithmic feel for time)
- **Control:** Knob
- **DSP Purpose:** Time between grain triggers. When tempo sync is enabled, snaps to note divisions.

### grainSize
- **ID:** grainSize
- **Type:** Float
- **Range:** 5.0 to 500.0
- **Default:** 100.0
- **Unit:** ms
- **Skew:** 0.5 (logarithmic feel for time)
- **Control:** Knob
- **DSP Purpose:** Length of each grain window. Affects texture density and character.

### feedback
- **ID:** feedback
- **Type:** Float
- **Range:** 0.0 to 100.0
- **Default:** 30.0
- **Unit:** %
- **Skew:** 1.0 (linear)
- **Control:** Knob
- **DSP Purpose:** Amount of processed output fed back into the grain buffer for repeating delays.

### chaos
- **ID:** chaos
- **Type:** Float
- **Range:** 0.0 to 100.0
- **Default:** 25.0
- **Unit:** %
- **Skew:** 1.0 (linear)
- **Control:** Knob
- **DSP Purpose:** Master randomization amount. Scales randomization of grain position, pitch (octaves/fifths), pan, and density.

### character
- **ID:** character
- **Type:** Float
- **Range:** 0.0 to 100.0
- **Default:** 50.0
- **Unit:** %
- **Skew:** 1.0 (linear)
- **Control:** Knob
- **DSP Purpose:** Morphs between glitchy (0% - sparse grains, short crossfades) and smooth/angelic (100% - dense grains, long crossfades).

### mix
- **ID:** mix
- **Type:** Float
- **Range:** 0.0 to 100.0
- **Default:** 50.0
- **Unit:** %
- **Skew:** 1.0 (linear)
- **Control:** Knob
- **DSP Purpose:** Dry/wet balance between original signal and processed granular output.

### tempoSync
- **ID:** tempoSync
- **Type:** Bool
- **Default:** true
- **Control:** Toggle
- **DSP Purpose:** When enabled, delay time snaps to host tempo note divisions (1/16, 1/8, 1/4, 1/2, 1).

## Technical Notes

- Pitch randomization constrained to octaves (+/- 12 semitones) and fifths (+/- 7 semitones)
- Mono input, stereo output with randomized pan per grain
- Grain envelope crossfade time controlled by character parameter
- Window functions (Hann or Tukey) for smooth grain envelopes

## APVTS Layout

```cpp
// Parameter IDs for AudioProcessorValueTreeState
static constexpr const char* DELAY_TIME_ID = "delayTime";
static constexpr const char* GRAIN_SIZE_ID = "grainSize";
static constexpr const char* FEEDBACK_ID = "feedback";
static constexpr const char* CHAOS_ID = "chaos";
static constexpr const char* CHARACTER_ID = "character";
static constexpr const char* MIX_ID = "mix";
static constexpr const char* TEMPO_SYNC_ID = "tempoSync";
```
