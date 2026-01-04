# OrganicHats Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.0
- **Type:** Synth (Instrument)
- **Complexity:** 5.0 (Complex)

## Lifecycle Timeline

- **2025-11-12:** Creative brief completed
- **2025-11-12 (Stage 0):** Research completed - DSP architecture documented
- **2025-11-12 (Stage 1):** Planning - Complexity 5.0 (phased implementation)
- **2025-11-12 (Stage 2):** Foundation - Build system operational, compiles successfully
- **2025-11-12 (Stage 3):** Shell complete - 6 parameters implemented
- **2025-11-12 (Stage 4.1):** Voice architecture + noise generation (C1/D1 MIDI routing, ADSR envelopes)
- **2025-11-12 (Stage 4.2):** Filtering + tone shaping (Tone filter 3-15kHz, Noise Color filter 5-10kHz, velocity modulation)
- **2025-11-12 (Stage 4.3):** Resonators + choke logic (3 fixed peaks 7/10/13kHz, closed chokes open)
- **2025-11-12 (Stage 5.1):** WebView layout integration (600×590px, studio hardware aesthetic)
- **2025-11-12 (Stage 5.2):** Parameter binding complete (6 parameters with ES6 modules and relative drag)
- **2025-11-12 (Stage 5.3):** Power LED and polish (pulse animation, final styling)
- **2025-11-12 (Stage 6):** Validation complete - Build verified, CHANGELOG created
- **2025-11-12 (v1.0.0):** Installed to system folders (VST3 + AU)

## Known Issues

None

## Additional Notes

**Description:**
Warm, organic noise-based hi-hat synthesizer with separate closed and open sounds. Inspired by CR-78/808 drum machines but with warmer, less metallic character. Features instant choking behavior (closed chokes open).

**Parameters (8 total):**

Closed Hi-Hat:
- Tone: 0-100%, default 50% (brightness control)
- Decay: 20-200ms, default 80ms (envelope decay time)
- Noise Color: 0-100%, default 50% (warmth vs brightness filtering)

Open Hi-Hat:
- Tone: 0-100%, default 50% (brightness control, shared character with closed)
- Release: 100ms-1s, default 400ms (sustained decay time)
- Noise Color: 0-100%, default 50% (warmth vs brightness filtering)

Global:
- Velocity > Volume: 0-100%, default 100% (velocity sensitivity to volume)
- Velocity > Tone: 0-100%, default 50% (velocity sensitivity to brightness)

**DSP:** Filtered noise synthesis with fixed resonance peaks for organic body. Static noise generation (no random variation). Closed and open sounds use separate MIDI note triggers. Instant choke behavior (<5ms) when closed chokes open. Velocity affects both volume and tone brightness.

**GUI:** Dual-panel layout with separate sections for closed (left) and open (right) hi-hats. Global controls section. Visual indication of active sound and choke behavior. User preset save/recall capability.

**MIDI:** Different MIDI notes for closed and open (e.g., C1 = closed, D1 = open). Velocity controls volume and tone brightness. Closed hi-hat trigger instantly cuts open hi-hat.

**Presets:** User preset save/recall only (no factory presets).

**Use Cases:**
- Electronic music production requiring warm, synthesized hi-hat sounds
- Layering with acoustic drum samples for synthetic character
- Creating unique percussive textures blending organic warmth with electronic precision
- Alternative to harsh digital hi-hat samples in house, techno, and ambient

**Inspirations:**
- CR-78/808 drum machines (classic electronic character but warmer)
- Organic analog drum machines (Jomox, Vermona)

**Formats:** VST3, AU, Standalone

**Installation Locations:**
- VST3: `~/Library/Audio/Plug-Ins/VST3/OrganicHats.vst3` (3.5 MB)
- AU: `~/Library/Audio/Plug-Ins/Components/OrganicHats.component` (3.4 MB)
