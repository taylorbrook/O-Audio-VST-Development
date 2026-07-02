# DrumRoulette Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.0
- **Type:** Instrument (Drum Sampler)
- **Complexity:** 5.0 / 5.0 (maximum - phased implementation)

## Lifecycle Timeline

- **2025-11-12:** Creative brief completed
- **2025-11-12 (Stage 0):** Research completed - DSP architecture documented
- **2025-11-12 (Stage 1):** Planning - Complexity 5.0 (phased implementation: 4 DSP phases + 3 GUI phases)
- **2025-11-12 (Stage 2):** Foundation - Build system operational, compiles successfully
- **2025-11-12 (Stage 3):** Shell complete - 73 parameters implemented (65 APVTS + 8 deferred to Stage 4)
- **2025-11-12 (Stage 4.1):** Voice architecture + sample playback - 8 voices, MIDI mapping C1-G1, multi-output (18 channels)
- **2025-11-12 (Stage 4.2):** Envelope + pitch shifting - ADSR per voice (sustain=0), pitch ±12 semitones, linear interpolation
- **2025-11-12 (Stage 4.3):** Tilt filter + volume control - cascaded shelving filters at 1kHz, per-voice gain (-60dB to +6dB)
- **2025-11-12 (Stage 4.4):** Folder randomization + solo/mute - recursive scanning, random selection, priority routing
- **2025-11-12 (Stage 4 COMPLETE):** All DSP components functional - ready for GUI integration
- **2025-11-12 (Stage 5 Phase 5.1):** WebView layout setup - 8 channel strips, JUCE patterns applied, zero errors
- **2025-11-12 (Stage 5 Phase 5.2):** Parameter binding slots 1-2 - 16 parameters bound, correct member order, zero errors
- **2025-11-12 (Stage 5 Phase 5.3):** Parameter binding slots 3-8 - 48 parameters bound, 100% contract compliance
- **2025-11-12 (Stage 5 COMPLETE):** All GUI integration finished - 64 parameters bound, all JUCE 8 patterns applied
- **2025-11-12 (Stage 6):** Validation complete - 5 factory presets created, CHANGELOG.md generated, ready for installation

## Known Issues

None

## Additional Notes

**Description:**
Eight-slot drum sampler with folder-based randomization and mixer-style interface. Each slot connects to a folder for random sample selection, with per-channel lock controls and multi-output routing.

**Parameters (73 total):**

Global:
- Randomize All: Button (randomize all unlocked slots)

Per-Slot (×8):
- Folder Path: File Browser
- Randomize: Button (pick random sample from folder)
- Lock: Toggle (exclude from global randomization)
- Volume: Fader (-inf to +6dB), default 0dB
- Decay: 10ms-2s, default 500ms (envelope decay, sustain=0)
- Attack: 0-50ms, default 1ms (percussive shaping)
- Tilt Filter: -12dB to +12dB, default 0dB (brightness, pivot at 1kHz)
- Pitch: ±12 semitones, default 0
- Solo: Toggle, default Off
- Mute: Toggle, default Off

**DSP:** Sample playback engine using juce::Synthesiser with 8 custom voices. ADSR envelope (sustain=0). Tilt filter (cascaded low/high shelving filters at 1kHz pivot). Pitch shifting via variable-rate resampling with linear interpolation. True random file selection with recursive folder scanning. Multi-output routing (18 channels: 2 main + 16 individual).

**GUI:** Eight vertical channel strips (mixer console style). LED-style trigger indicators. Color-coded lock icons (grey=unlocked, highlighted=locked). Global randomize button. Vintage hardware mixer aesthetic with brushed metal texture (1400×950px).

**MIDI:** C1-G1 (8 chromatic notes), velocity-sensitive.

**Audio Routing:** Automatic multi-output (stereo main + 8 individual stereo outputs = 18 total).

**Folder Handling:**
- Recursive subfolder scanning
- True random selection (any file each time)
- Error message on empty folders
- Manual folder selection each session

**Validation:**
- ✓ Factory presets: 5 presets created (Default Kit, Tight Drums, Dark and Deep, Bright and Crispy, Lo-Fi Character)
- ✓ CHANGELOG.md: Generated in Keep a Changelog format
- ✓ Build verification: Compiles successfully (VST3, AU, Standalone) - 5.5MB Release binary
- ✓ Parameter count: 73 parameters (1 global + 72 per-slot) - matches parameter-spec.md exactly
- ✓ DSP chain: All 4 phases verified (Voice architecture, Envelope+Pitch, Tilt+Volume, Randomization+Solo/Mute)
- ✓ GUI bindings: 64 parameter attachments (8 slots × 8 params) - 100% contract compliance

**Formats:** VST3, AU, Standalone

**Installation Locations:**
- VST3: `~/Library/Audio/Plug-Ins/VST3/DrumRoulette.vst3` (5.5 MB)
- AU: `~/Library/Audio/Plug-Ins/Components/DrumRoulette.component` (5.4 MB)
