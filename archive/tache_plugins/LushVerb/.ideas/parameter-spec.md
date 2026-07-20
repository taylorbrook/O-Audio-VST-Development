# LushVerb - Parameter Specification

**Generated:** 2025-11-11
**From:** UI Mockup v3
**Status:** 🔒 LOCKED - This specification is immutable during implementation

## Total Parameter Count

**Total:** 4 parameters

## Parameter Definitions

### SIZE
- **Type:** Float
- **Range:** 0.5 to 20.0 seconds
- **Default:** 2.5s
- **Unit:** seconds
- **Skew Factor:** Logarithmic (suggested for time-based parameters)
- **UI Control:** Rotary knob (70px diameter), left-most position
- **Display Format:** "{value:.1f}s" (e.g., "2.5s", "10.0s")
- **DSP Usage:** Reverb tail length - controls room size and decay time

### DAMPING
- **Type:** Float
- **Range:** 0.0 to 100.0 percent
- **Default:** 30.0%
- **Unit:** percent
- **Skew Factor:** Linear
- **UI Control:** Rotary knob (70px diameter), second position
- **Display Format:** "{value:.0f}%" (e.g., "30%", "75%")
- **DSP Usage:** High-frequency rolloff - darker/brighter tail character

### SHIMMER
- **Type:** Float
- **Range:** 0.0 to 100.0 percent
- **Default:** 30.0%
- **Unit:** percent
- **Skew Factor:** Linear
- **UI Control:** Rotary knob (70px diameter), third position
- **Display Format:** "{value:.0f}%" (e.g., "30%", "80%")
- **DSP Usage:** +1 octave pitch-shifted signal amount (0% = disabled, higher = more shimmer)

### MIX
- **Type:** Float
- **Range:** 0.0 to 100.0 percent
- **Default:** 30.0%
- **Unit:** percent
- **Skew Factor:** Linear
- **UI Control:** Rotary knob (70px diameter), right-most position
- **Display Format:** "{value:.0f}%" (e.g., "30%", "100%")
- **DSP Usage:** Dry/wet blend (0% = fully dry, 100% = fully wet)

## UI Layout Specification

### Window
- **Dimensions:** 500×300px
- **Resizable:** No

### Control Layout
- **Type:** Horizontal row of 4 knobs
- **Knob diameter:** 70px
- **Spacing:** Evenly distributed
  - SIZE center: 75px (left edge aligned with meter at 40px)
  - DAMPING center: 191.67px
  - SHIMMER center: 308.33px
  - MIX center: 425px (right edge aligned with meter at 460px)

### Visual Feedback
- **LED Output Meter:** 20 LEDs, 420px wide, positioned at x:40px
  - Green zone: -60dB to -12dB
  - Yellow zone: -12dB to -3dB
  - Red zone: -3dB to 0dB

### Color Palette
- **Background:** Dark rack metal (#1a1a1a, #1e1e1e)
- **Accents:** Warm gold (#ffc864)
- **Text:** Light gray (#cccccc)
- **LED colors:** Green (#44ff44), Yellow (#ffff44), Red (#ff4444)

## Parameter Notes

### SIZE Parameter
- Logarithmic scaling recommended for musical time perception
- Range extends to 20s+ for infinite ambient textures
- Small values (0.5-2s) for tight mixing reverb
- Large values (10-20s) for creative soundscapes

### DAMPING Parameter
- Controls only high-frequency rolloff, NOT decay rate
- 0% = bright, airy, crystalline (full high-frequency content)
- 100% = dark, warm, diffuse (maximum HF attenuation)
- Independent of SIZE parameter

### SHIMMER Parameter
- Evolution from original "always-on" design to user-controllable
- Default 30% preserves subtle shimmer character
- 0% disables shimmer entirely (traditional reverb)
- Higher values create ambient/pad-like textures
- Pitch-shifted +1 octave above input signal

### MIX Parameter
- Standard dry/wet blend
- 0% = bypass (fully dry)
- 100% = fully wet reverb signal
- Default 30% balances reverb effect with source

## Implementation Contract

This specification is the **single source of truth** for Stages 1-4:

- **Stage 1 (Foundation):** Build system and APVTS creation must match these exact specs
- **Stage 2 (DSP):** Audio processing references these ranges and defaults
- **Stage 3 (GUI):** WebView UI binds to these parameter IDs
- **Stage 4 (Validation):** Factory presets use these ranges

**No deviations allowed during implementation.** Any changes require returning to Stage 0 and updating this specification.
