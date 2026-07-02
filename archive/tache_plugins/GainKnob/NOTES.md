# GainKnob Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.2.3
- **Type:** Audio Effect (Utility)

## Lifecycle Timeline

- **2025-11-10:** Creative brief completed
- **2025-11-10 (Stage 0):** Research completed - DSP architecture documented
- **2025-11-10 (Stage 1):** Planning - Complexity 1.2 (single-pass implementation)
- **2025-11-10 (Stage 2):** Foundation - Build system operational, compiles successfully
- **2025-11-10 (Stage 3):** Shell complete - 1 parameter implemented
- **2025-11-10 (Stage 4):** DSP complete - Gain conversion with denormal protection
- **2025-11-10 (Stage 5):** GUI complete - WebView integration with v1 mockup
- **2025-11-10 (Stage 6):** Validation complete - ready for installation
- **2025-11-10 (v1.0.0):** Installed to system folders (VST3 + AU)
- **2025-11-10 (v1.1.0):** Pan parameter added - constant power panning, dual knob UI
- **2025-11-10 (v1.2.0):** DJ-style filter added - Butterworth IIR filters, three knob UI
- **2025-11-10 (v1.2.1):** Filter frequency curve fixed - exponential mapping for musical sweep
- **2025-11-10 (v1.2.2):** Filter direction inverted - center position now bypasses correctly
- **2025-11-10 (v1.2.3):** Filter state burst eliminated - reset delay buffers on type transitions

## Known Issues

None

## Additional Notes

**Description:**
Minimalist gain, pan, and DJ-style filter utility plugin with three knobs for volume attenuation, stereo positioning, and frequency filtering.

**Parameters (3 total):**
- Gain: -∞ to 0dB, default 0dB (volume attenuation)
- Pan: -100% L to +100% R, default 0% (stereo positioning)
- Filter: -100% to +100%, default 0% (DJ-style filter: negative=low-pass, positive=high-pass, 0%=bypass)

**DSP:** 2nd-order Butterworth IIR filters (200Hz-20kHz), gain multiplication, constant power panning. DSP chain: Filter → Gain → Pan.

**GUI:** Three horizontal rotary knobs with value displays. Clean, minimal design. 800x400px.

**Validation:**
- ✓ Factory presets: 5 presets created (Unity, Subtle Cut, Half Volume, Quiet, Silence)
- ✓ CHANGELOG.md: Generated in Keep a Changelog format
- ✓ Parameter spec adherence: GAIN parameter matches spec exactly (-60 to 0 dB)
- ✓ DSP implementation: Decibels::decibelsToGain with silence at minimum
- ✓ WebView UI: Knob binding operational

**Formats:** VST3, AU, Standalone

**Installation Locations:**
- VST3: `~/Library/Audio/Plug-Ins/VST3/GainKnob.vst3` (4.1 MB)
- AU: `~/Library/Audio/Plug-Ins/Components/GainKnob.component` (4.0 MB)
