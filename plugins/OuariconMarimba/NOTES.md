# Ouaricon Marimba Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.3.0
- **Type:** Synth (Physical Model)
- **Complexity:** 5.0 (VERY HIGH - maximum complexity)

## Installation Locations
- **VST3:** `~/Library/Audio/Plug-Ins/VST3/Ouaricon Marimba.vst3`
- **AU:** `~/Library/Audio/Plug-Ins/Components/Ouaricon Marimba.component`

## Lifecycle Timeline

- **2026-01-09 (Stage 0):** Research & Planning complete - Architecture and plan documented (Complexity 5.0)
- **2026-01-09 (v1.0.0):** Released and installed
  - Modal synthesis with 8 resonant modes
  - Full tuning system: 12-TET, Scala file loading, custom interval editing
  - WebView UI with botanical paper aesthetic
  - Playable keyboard in tuning tab
  - Dynamic interval list and pitch circle (adapts to any scale size)
- **2026-01-10 (v1.1.0):** Microtonal UI improvements
  - Circular scale indicator flashes red for ALL notes (GUI, external MIDI, DAW)
  - A4 reference pitch dial resets to 440 Hz on double-click
  - Fixed keyboard: black keys restructured as siblings for proper click detection
- **2026-01-10 (v1.2.0):** Tonic selection + bug fix
  - Added tonic note selector (click to cycle C→C#→D→...→B→C)
  - Fixed: UI keyboard no longer causes permanent red indicator lines (race condition)
- **2026-01-10 (v1.2.1):** Tonic UI fixes
  - Bi-directional arrows (left=down, right=up)
  - Keyboard always shows C-D-E-F-G-A-B (physical layout)
- **2026-01-10 (v1.2.2):** Tonic interval reference fix
  - Tonic sets root (1/1) for interval calculations, not transposition
  - In Just Intonation: intervals calculated from tonic note
  - In 12-TET: no audible effect (equal temperament)
- **2026-01-10 (v1.2.3):** Oscilloscope + label improvements
  - Fixed: Waveform display now functions as live oscilloscope
  - Renamed: "MALLET" → "MALLET HARDNESS", "MATERIAL" → "MATERIAL HARDNESS"
- **2026-01-10 (v1.2.4):** Velocity dynamics enhancement
  - Changed: +6dB boost at max velocity (vel 1 unchanged, smooth scaling)
  - Independent of VEL_CURVE parameter - more expressive dynamics
- **2026-01-10 (v1.2.5):** Live VU meter
  - Added: VU meter now responds to audio output in real-time
  - Ballistic needle motion (fast attack, slow decay)
  - Scale: -60dB to +3dB with full semicircle sweep
  - Dynamic needle color gradient: green (quiet) → red (loud)
- **2026-01-10 (v1.2.6):** Polyphonic interval highlighting
  - Fixed: Scale interval indicators now highlight ALL held notes (chords)
  - Velocity-based intensity: harder hits = brighter red
  - Lock-free MIDI event queue for proper note-on/note-off tracking
- **2026-01-10 (v1.3.0):** Preset system
  - Added: Complete preset save/load system with factory presets
  - Factory presets: Default Marimba, Bright Marimba, Soft Marimba, Just Intonation, Pythagorean, Quarter-Comma Meantone, Baroque A=415, Concert A=442
  - User presets stored in ~/Library/Application Support/Ouaricon Marimba/Presets/User/
  - DAW session state now includes full tuning configuration
  - Preset browser: ◀ ▶ navigation, SAVE button with file dialog
  - JSON format for easy editing/sharing
- **2026-01-10 (v1.3.1):** Preset UX improvements
  - Added: LOAD button for file dialog preset loading
  - Added: Dropdown menu when clicking preset name (instant preset selection)
  - Dropdown shows Factory/User sections with active preset highlighted

## Known Issues

None

## Additional Notes

### Description
Physically modeled marimba synthesizer with native microtonal support using modal synthesis, convolution body resonance, and comprehensive tuning system integration.

### Key Features
- **Modal Synthesis:** 8 resonant modes per voice with inharmonic overtones (authentic marimba character)
- **Microtonality:** Native support for Scala files, custom interval editing, and 12-TET
- **Preset System:** Factory presets + user presets (JSON format), complete patch state including tuning
- **Body Resonance:** Convolution IR for resonator tube coupling
- **Voice Management:** 16-24 polyphony with efficient voice stealing
- **Velocity Response:** Custom curve mapping for expressive performance
- **WebView UI:** Botanical paper aesthetic with interactive tuning visualization

### Parameters (7 total)
1. **MALLET_HARDNESS** (Float, 0.0-1.0) - Excitation brightness (soft/dark to hard/bright)
2. **BAR_MATERIAL** (Float, 0.0-1.0) - Spectral balance (rosewood to synthetic)
3. **RESONANCE** (Float, 0.0-1.0) - Decay time + body IR mix
4. **TUNING_MODE** (Choice, 0-2) - 12-TET / Scala / MTS-ESP
5. **REFERENCE_PITCH** (Float, 400-480 Hz) - A4 reference frequency
6. **VEL_CURVE** (Float, 0.0-1.0) - Velocity curve (linear to exponential)
7. **OUTPUT_GAIN** (Float, -24 to +12 dB) - Master output level

### DSP Architecture
- **Modal Resonator Bank:** 8 parallel 2nd-order IIR biquad filters per voice
  - Mode ratios: 1.00, 3.93, 9.24, 16.65, 26.3, 38.2, 52.4, 68.9 (inharmonic)
- **Mallet Exciter:** Filtered noise burst (5-20ms) with velocity-dependent brightness
- **Body Resonance:** juce::dsp::Convolution with short IR
- **Tuning Engine:** 12-TET, Scala files, custom intervals with ratio/cents input

### UI Features
- **Sound Tab:** Mallet Hardness, Material Hardness, Resonance knobs; Velocity curve display; Output level with VU meter; Live oscilloscope display
- **Tuning Tab:** Mode selection (12-TET/Scala/MTS-ESP); Reference pitch knob; Scala file loading; Interactive pitch circle; Editable interval list (supports any scale size); Tonic note selector; Playable 1-octave keyboard

### Formats
- VST3
- AU (Audio Unit)
- Standalone

**Last Updated:** 2026-01-10
