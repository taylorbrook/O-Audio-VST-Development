# FlutterVerb - Parameter Specification

**Version:** 1.0 (from mockup v6)
**Created:** 2025-11-11
**Status:** Locked (immutable contract for implementation)

## Total Parameter Count

**Total:** 7 parameters (6 continuous + 1 toggle)

## Parameter Definitions

### SIZE
- **Type:** Float
- **Range:** 0.0 to 100.0 (%)
- **Default:** 50.0
- **Skew Factor:** Linear
- **UI Control:** Rotary knob (TapeAge style), top row left
- **DSP Usage:** Room dimensions (affects early reflections and perceived space size)

### DECAY
- **Type:** Float
- **Range:** 0.1 to 10.0 (seconds)
- **Default:** 2.5
- **Skew Factor:** Linear (or exponential for musical feel)
- **UI Control:** Rotary knob (TapeAge style), top row center
- **DSP Usage:** Reverb tail length (wide range for tight spaces to ambient washes)

### MIX
- **Type:** Float
- **Range:** 0.0 to 100.0 (%)
- **Default:** 25.0
- **Skew Factor:** Linear
- **UI Control:** Rotary knob (TapeAge style), top row right
- **DSP Usage:** Dry/wet blend of reverb signal

### AGE
- **Type:** Float
- **Range:** 0.0 to 100.0 (%)
- **Default:** 20.0
- **Skew Factor:** Linear
- **UI Control:** Rotary knob (TapeAge style), bottom row left
- **DSP Usage:** Tape character intensity (combines wow and flutter modulation depth for vintage tape feel)

### DRIVE
- **Type:** Float
- **Range:** 0.0 to 100.0 (%)
- **Default:** 20.0
- **Skew Factor:** Linear
- **UI Control:** Rotary knob (TapeAge style), bottom row center
- **DSP Usage:** Tape saturation/warmth amount (adjustable analog coloration)

### TONE
- **Type:** Float
- **Range:** -100.0 to +100.0
- **Default:** 0.0
- **Skew Factor:** Linear
- **UI Control:** Rotary knob (TapeAge style), bottom row right
- **DSP Usage:** DJ-style filter (center=full, left=low-pass, right=high-pass) based on GainKnob implementation

### MOD_MODE
- **Type:** Boolean (Choice)
- **States:** 0 = "WET ONLY", 1 = "WET+DRY"
- **Default:** 0 (WET ONLY)
- **UI Control:** Horizontal toggle switch (below Tone knob)
- **DSP Usage:** Applies modulation to dry signal when enabled (wet+dry mode)

## UI Layout

**Window Size:** 600×640px (non-resizable)

**Layout Structure:**
- VU meter at top (horizontal, 480×100px, output peak monitoring)
- Title centered below VU meter
- Two rows of 3 knobs (aligned with VU meter edges)
  - Top row: Size, Decay, Mix (reverb controls)
  - Bottom row: Age, Drive, Tone (tape character controls)
- Toggle switch below Tone knob (Mod Mode)

**Visual Style:**
- Dark radial gradient background (#4a3a2a → #2a1a0a)
- TapeAge-style knobs (dark brown with brass pointer)
- Brass/gold accents (#d4a574, #c49564)
- Typewriter/monospace typography
- Scan lines and vignette effects for vintage feel

## Signal Flow

```
Input
  ↓
Plate Reverb (SIZE, DECAY)
  ↓
Wow/Flutter Modulation (AGE)
  ↓
Tape Saturation (DRIVE)
  ↓
DJ-style Filter (TONE)
  ↓
Dry/Wet Blend (MIX)
  ↓
Output
  ↓
VU Meter

MOD_MODE: When enabled, applies AGE modulation to dry signal as well
```

## DSP Implementation Notes

### Size Parameter
- Controls early reflection density and spacing
- Affects perceived room dimensions
- Algorithm: Plate reverb room size parameter

### Decay Parameter
- Controls reverb tail length
- Range: 0.1s (tight spaces) to 10.0s (ambient washes)
- Algorithm: Plate reverb decay time

### Mix Parameter
- Standard dry/wet blend
- 0% = fully dry, 100% = fully wet
- Algorithm: Linear crossfade

### Age Parameter
- Single parameter controlling wow and flutter depth
- Combines two LFO pitch modulators (different rates)
- 0% = no modulation, 100% = extreme warbling
- Algorithm: Dual LFO with pitch shifting

### Drive Parameter
- Tape saturation/warmth
- Soft-clipping waveshaper with harmonic enhancement
- 0% = clean, 100% = saturated
- Algorithm: Soft clipper with even harmonics

### Tone Parameter
- DJ-style filter (exponential mapping)
- Negative values: Low-pass filter (-100% = 200Hz, 0% = 20kHz)
- Positive values: High-pass filter (0% = 20Hz, +100% = 10kHz)
- Center (0%): Full range bypass
- Algorithm: Butterworth IIR filters with exponential cutoff mapping
- Reference: GainKnob implementation

### MOD_MODE Parameter
- Boolean choice between two modulation routing modes
- WET ONLY (0): Age modulation applied only to reverb wet signal
- WET+DRY (1): Age modulation applied to both wet and dry signals
- Algorithm: Routing switch in DSP chain

## Validation Checklist

- [ ] All 7 parameters present in APVTS
- [ ] Parameter ranges match specification exactly
- [ ] Default values match specification
- [ ] UI controls bound to correct parameters
- [ ] Signal flow matches architecture diagram
- [ ] MOD_MODE routing implemented correctly
- [ ] VU meter reads output peak level
- [ ] DJ-style filter uses GainKnob exponential mapping

## Version History

**v1.0 (2025-11-11):** Initial specification from mockup v6
- 7 parameters finalized
- Layout: 600×640px with VU meter, 6 knobs, 1 toggle
- Visual style: Dark gradient with brass accents (TapeAge-inspired)
