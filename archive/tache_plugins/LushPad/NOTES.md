# LushPad Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.0
- **Type:** Synth (Instrument)

## Lifecycle Timeline

- **2025-11-13 (Stage 0):** Planning and research completed
- **2025-11-13 (Stage 2):** Foundation and shell implemented
- **2025-11-13 (Stage 3):** DSP implementation completed
- **2025-11-13 (Stage 4):** GUI integration completed
- **2025-11-13 (Stage 5):** Validation complete, installed to system
- **2025-11-13 (v1.0.0):** Fixed IS_SYNTH configuration issue - plugin now properly categorized as instrument with MIDI routing

## Known Issues

**Fixed:**
- ✅ Missing IS_SYNTH flag in CMakeLists.txt (v1.0.0) - Plugin was categorized as effect instead of instrument, preventing MIDI routing. Added IS_SYNTH TRUE and NEEDS_MIDI_INPUT TRUE.

## Additional Notes

**Description:** Lush ambient pad synthesizer with evolving textures and nested LFO modulation system.

**Parameters:**
1. Timbre (0.0-1.0) - Controls FM feedback depth and harmonic saturation
2. Filter Cutoff (20-20000 Hz) - Low-pass filter with velocity scaling
3. Reverb Amount (0.0-1.0) - Wet/dry mix for built-in reverb

**DSP Features:**
- 8-voice polyphony with oldest-note stealing
- 3 detuned oscillators per voice (±7 cents)
- FM feedback modulation
- Nested 9-LFO system per voice (primary/secondary/tertiary modulation)
- Velocity-sensitive low-pass filtering
- Per-voice filtering and panning
- Global stereo reverb

**GUI:** WebView-based UI with animated parameter controls

**Validation:** Stage 5 complete

**Formats:** VST3, AU, Standalone

**Installation:**
- VST3: ~/Library/Audio/Plug-Ins/VST3/LushPad.vst3
- AU: ~/Library/Audio/Plug-Ins/Components/LushPad.component

**Use Cases:** Ambient music, soundscapes, evolving pads, lush textures
