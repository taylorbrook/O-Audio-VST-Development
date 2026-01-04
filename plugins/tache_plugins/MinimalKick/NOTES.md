# MinimalKick Notes

## Status
- **Current Status:** 🚧 Stage 5
- **Version:** N/A
- **Type:** Synth
- **Complexity:** 5.0 (Complex - phased implementation)

## Lifecycle Timeline

- **2025-11-12:** Creative brief completed
- **2025-11-12:** UI mockup v2 finalized (5 knobs, vintage hardware aesthetic)
- **2025-11-12 (Stage 0):** Research completed - DSP architecture documented (Tier 4, 6 features)
- **2025-11-13 (Stage 1):** Planning completed - Complexity 5.0, 6-phase implementation plan created
- **2025-11-13 (Stage 2):** Foundation complete - Project structure created, JUCE 8 patterns followed
- **2025-11-13 (Stage 3):** Shell complete - 5 APVTS parameters implemented (sweep, time, attack, decay, drive)
- **2025-11-13 (Stage 4 Phase 4.1):** Core synthesis complete - Oscillator + MIDI + amplitude envelope (attack/decay working)
- **2025-11-13 (Stage 4 Phase 4.2):** Pitch envelope complete - Exponential decay with sweep/time parameters (808-style kick working)
- **2025-11-13 (Stage 4 Phase 4.3):** Saturation complete - Tanh waveshaping adds warm harmonics (drive parameter 0-100%)
- **2025-11-13 (Stage 4):** DSP complete - All 3 phases finished (core synthesis + pitch envelope + saturation)
- **2025-11-13 (Stage 5 Phase 5.1):** WebView layout complete - All 5 knobs rendering (730×280px, vintage hardware aesthetic)
- **2025-11-13 (Stage 5 Phase 5.2):** Parameter binding complete - All 5 parameters bound with bidirectional sync (UI ↔ DSP)

## Known Issues

None

## Additional Notes

**Description:**
Minimal house kick drum synthesizer with sine wave + pitch envelope architecture for deep, subby kicks that sit perfectly in minimal and tech house tracks.

**Parameters (5 total):**
- Sweep: 0-24 semitones, default 12 st (pitch envelope starting offset)
- Time: 5-500 ms, default 50 ms (pitch envelope decay time)
- Attack: 0-50 ms, default 5 ms (amplitude envelope attack)
- Decay: 50-2000 ms, default 400 ms (amplitude envelope decay)
- Drive: 0-100%, default 20% (saturation/harmonics)

**DSP:** Sine oscillator + exponential pitch envelope + AD amplitude envelope + tanh saturation. Monophonic, retriggerable. Estimated CPU: ~11% single core.

**Implementation Strategy:** Phased (6 phases: 3 DSP + 3 GUI)
- Stage 4.1: Core synthesis (oscillator + MIDI + amplitude)
- Stage 4.2: Pitch envelope (custom exponential - highest risk)
- Stage 4.3: Saturation/drive
- Stage 5.1-5.3: WebView UI (layout, binding, polish)
