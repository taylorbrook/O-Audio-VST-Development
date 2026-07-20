# MinimalKick - Parameter Specification

**Generated:** 2025-11-12
**From:** UI Mockup v2
**Status:** Locked for implementation

---

## Total Parameter Count

**Total:** 5 parameters

---

## Parameter Definitions

### sweep
- **Type:** Float
- **Range:** 0.0 to 24.0 semitones
- **Default:** 12.0
- **Skew Factor:** Linear
- **UI Control:** Rotary knob, position 1 (leftmost)
- **DSP Usage:** Pitch envelope amount (starting pitch offset from MIDI note)
- **Description:** Controls how many semitones above the base pitch the envelope starts

### time
- **Type:** Float
- **Range:** 5.0 to 500.0 ms
- **Default:** 50.0
- **Skew Factor:** Logarithmic (prefer log scale for time)
- **UI Control:** Rotary knob, position 2
- **DSP Usage:** Pitch envelope decay time
- **Description:** Time for pitch envelope to decay from Sweep offset to base pitch (short = clicky, long = laser)

### attack
- **Type:** Float
- **Range:** 0.0 to 50.0 ms
- **Default:** 5.0
- **Skew Factor:** Linear
- **UI Control:** Rotary knob, position 3
- **DSP Usage:** Amplitude envelope attack time
- **Description:** Attack time for amplitude envelope

### decay
- **Type:** Float
- **Range:** 50.0 to 2000.0 ms
- **Default:** 400.0
- **Skew Factor:** Logarithmic (prefer log scale for time)
- **UI Control:** Rotary knob, position 4
- **DSP Usage:** Amplitude envelope decay time (sustain always 0)
- **Description:** Decay time for amplitude envelope (kick-appropriate, no sustain)

### drive
- **Type:** Float
- **Range:** 0.0 to 100.0 %
- **Default:** 20.0
- **Skew Factor:** Linear
- **UI Control:** Rotary knob, position 5 (rightmost)
- **DSP Usage:** Saturation/distortion amount
- **Description:** Saturation/distortion for harmonics and warmth

---

## Parameter ID Naming Convention

All parameter IDs follow these rules:
- Lowercase only
- snake_case for multi-word parameters
- Alphanumeric + underscore only
- Max 32 characters
- Valid C++ identifier

---

## UI Layout

**Window size:** 730×280px (non-resizable)
**Layout:** 5 rotary knobs in horizontal row
**Spacing:** 50px gaps between knobs, 40px edge margins
**Knob size:** 90px diameter
**Aesthetic:** vintage-hardware-001 (warm browns, brass accents, scan lines)

---

## Implementation Notes

**Pitch envelope architecture:**
- Base pitch from MIDI note (standard synth)
- Sweep parameter adds semitone offset at note trigger
- Time parameter controls exponential decay to base pitch
- Phase-continuous to avoid clicks

**Amplitude envelope:**
- AD envelope (Attack + Decay)
- Sustain always 0 (kick-appropriate)
- No Release parameter needed

**Drive/saturation:**
- Applied after amplitude envelope
- Oversampling recommended to minimize aliasing
- Warm even-harmonic saturation preferred

---

## Change Log

- **2025-11-12:** Initial specification from UI mockup v2
