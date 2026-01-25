# O-Marimba Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.10.0
- **Type:** Synth (Physical Model)
- **Complexity:** 5.0 (VERY HIGH - maximum complexity)

## Installation Locations
- **VST3:** `~/Library/Audio/Plug-Ins/VST3/O-Marimba.vst3`
- **AU:** `~/Library/Audio/Plug-Ins/Components/O-Marimba.component`

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
  - User presets stored in ~/Library/Application Support/O-Marimba/Presets/User/
  - DAW session state now includes full tuning configuration
  - Preset browser: ◀ ▶ navigation, SAVE button with file dialog
  - JSON format for easy editing/sharing
- **2026-01-10 (v1.3.1):** Preset UX improvements
  - Added: LOAD button for file dialog preset loading
  - Added: Dropdown menu when clicking preset name (instant preset selection)
  - Dropdown shows Factory/User sections with active preset highlighted
- **2026-01-11 (v1.4.0):** Tuning export + UI fixes
  - Added: Export tuning as .scl (Scala) and .kbm (keyboard mapping) files
  - Changed: "SCALA" button renamed to "CUSTOM" for clarity
  - Fixed: Interval table now non-editable in 12-TET mode (was editable but non-functional)
- **2026-01-11 (v1.5.0):** Physical model realism improvements
  - Changed: Modal frequency ratios corrected from acoustic research (Euphonics/ISMA2019)
    - Mode 2 now 4.0x (tuned double octave - professional marimba signature)
    - Higher modes corrected to measured values: 24.22, 33.54, 42.97, 54.0
  - Changed: Mode amplitude distribution improved with stronger fundamental + mode 2
  - Changed: Body resonance IR enhanced with 6 modes (was 3), wood-like decay envelope, 100ms duration
- **2026-01-11 (v1.6.0):** Three new timbre refinement controls (Strike Position, Overtone Damping, Tone)
- **2026-01-11 (v1.6.1):** UI refinement + doubled parameter ranges for extreme sound shaping
- **2026-01-11 (v1.6.2):** UI knob sizing fixes, updated factory presets with new parameters
- **2026-01-13 (v1.7.0):** New EFFECTS tab
  - Added: Third tab in UI for future effects controls (placeholder)
  - Added: Parallax tree background shifts further left on Effects tab
  - Tree progression: Sound (-60px) → Tuning (-120px) → Effects (-180px)
- **2026-01-13 (v1.8.0):** Analog EQ Module (first Ouaricon Module integration)
  - Added: 4-band analog-style parametric EQ in Effects tab
    - LF Shelf (30-500 Hz), LMF Bell (100-2000 Hz), HMF Bell (500-8000 Hz), HF Shelf (2000-20000 Hz)
    - Variable Q selection (WIDE/MED/TIGHT) for LMF and HMF bands
    - Analog saturation circuit (toggleable)
    - Output gain trim (±12 dB)
  - Added: Dual-ring knob UI (outer=freq, inner=gain)
  - Signal flow: Synth → Body Resonance → Analog EQ → Output Gain
  - Uses Ouaricon Module System: modules/effects/analog-eq-unit/
  - 16 new automatable parameters (fx_eq_* prefix)
- **2026-01-14 (v1.9.0):** Compressor Module (second Ouaricon Module integration)
  - Added: Compact dynamics compressor below EQ in Effects tab
    - Threshold (-60 to 0 dB), Ratio (1:1 to 20:1)
    - Attack (0.1-100 ms), Release (10-1000 ms)
    - Fixed 6dB soft knee for musical response
    - Clickable title for bypass (ON by default)
    - Vertical GR LED meter with real-time updates
  - Signal flow: Synth → Body Resonance → EQ → **Compressor** → Output Gain
  - Uses Ouaricon Module System: modules/effects/compressor-unit/
  - 5 new automatable parameters (fx_comp_* prefix)
- **2026-01-14 (v1.9.1):** Compressor UI improvements
  - Changed: Compact single-row layout matching EQ module style
  - Added: COMP bypass button (left), AUTO (autogain) button (right)
  - Added: Autogain feature - automatic makeup gain compensation
  - Knobs centered and aligned with EQ bands above
  - 1 new parameter: fx_comp_autogain (bool)
- **2026-01-14 (v1.9.2):** Effects tab UI refinement
  - Changed: Both EQ and Compressor modules 10px shorter, 100px wider
  - Changed: Compressor knobs use canonical 10-segment Ouaricon seed pattern
  - Updated: analog-eq-unit module v1.2.0, compressor-unit module v1.2.0
- **2026-01-14 (v1.9.3):** Compressor clicking fix
  - Fixed: Clicking at high gain reduction caused by unsmoothed gain application
  - Added: Gain coefficient smoothing (GAIN_SMOOTH_COEFF = 0.005)
  - Updated: compressor-unit DSP to v1.2.1
- **2026-01-25 (v1.10.0):** Renamed plugin from "Ouaricon Marimba" to "O-Marimba"
  - Changed: Plugin display name, folder name, class names, preset paths
  - Note: DAW sessions using old name will need to re-load the plugin

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

### Parameters (32 total - v1.9.1)

**Sound Parameters (10):**
1. **MALLET_HARDNESS** (Float, 0.0-1.0) - Excitation brightness (soft/dark to hard/bright)
2. **BAR_MATERIAL** (Float, 0.0-1.0) - Spectral balance (rosewood to synthetic)
3. **RESONANCE** (Float, 0.0-1.0) - Decay time + body IR mix
4. **STRIKE_POSITION** (Float, 0.0-1.0) - Mallet strike location (edge to center)
5. **OVERTONE_DAMPING** (Float, 0.0-1.0) - Upper harmonic decay rate
6. **TONE** (Float, 0.0-1.0) - Post-synthesis brightness (lowpass filter)
7. **TUNING_MODE** (Choice, 0-2) - 12-TET / Scala / MTS-ESP
8. **REFERENCE_PITCH** (Float, 400-480 Hz) - A4 reference frequency
9. **VEL_CURVE** (Float, 0.0-1.0) - Velocity curve (linear to exponential)
10. **OUTPUT_GAIN** (Float, -24 to +12 dB) - Master output level

**Analog EQ Parameters (16, prefix: fx_eq_):**
11. **fx_eq_lf_freq** (Float, 30-500 Hz) - LF shelf corner frequency
12. **fx_eq_lf_gain** (Float, ±12 dB) - LF shelf boost/cut
13. **fx_eq_lf_on** (Bool) - LF band enable
14. **fx_eq_lmf_freq** (Float, 100-2000 Hz) - LMF bell center frequency
15. **fx_eq_lmf_gain** (Float, ±12 dB) - LMF bell boost/cut
16. **fx_eq_lmf_q** (Choice, WIDE/MED/TIGHT) - LMF Q factor
17. **fx_eq_lmf_on** (Bool) - LMF band enable
18. **fx_eq_hmf_freq** (Float, 500-8000 Hz) - HMF bell center frequency
19. **fx_eq_hmf_gain** (Float, ±12 dB) - HMF bell boost/cut
20. **fx_eq_hmf_q** (Choice, WIDE/MED/TIGHT) - HMF Q factor
21. **fx_eq_hmf_on** (Bool) - HMF band enable
22. **fx_eq_hf_freq** (Float, 2000-20000 Hz) - HF shelf corner frequency
23. **fx_eq_hf_gain** (Float, ±12 dB) - HF shelf boost/cut
24. **fx_eq_hf_on** (Bool) - HF band enable
25. **fx_eq_output_gain** (Float, ±12 dB) - EQ master output trim
26. **fx_eq_analog** (Bool) - Analog saturation enable

**Compressor Parameters (6):**
27. **fx_comp_enabled** (Bool) - Compressor bypass (ON by default)
28. **fx_comp_threshold** (Float, -60 to 0 dB) - Compression threshold
29. **fx_comp_ratio** (Float, 1:1 to 20:1) - Compression ratio
30. **fx_comp_attack** (Float, 0.1-100 ms) - Attack time
31. **fx_comp_release** (Float, 10-1000 ms) - Release time
32. **fx_comp_autogain** (Bool) - Automatic makeup gain (OFF by default)

### DSP Architecture
- **Modal Resonator Bank:** 8 parallel 2nd-order IIR biquad filters per voice
  - Mode ratios: 1.00, 4.00, 9.24, 16.27, 24.22, 33.54, 42.97, 54.0 (v1.5.0: research-corrected)
  - Mode 2 (4.0x) is the tuned double octave - signature of professional marimbas
- **Mallet Exciter:** Filtered noise burst (5-20ms) with velocity-dependent brightness
- **Body Resonance:** juce::dsp::Convolution with 100ms IR (6 resonant modes, wood-like decay)
- **Tuning Engine:** 12-TET, Scala files, custom intervals with ratio/cents input

### UI Features
- **Sound Tab:** Mallet Hardness, Material Hardness, Resonance knobs; Velocity curve display; Output level with VU meter; Live oscilloscope display
- **Tuning Tab:** Mode selection (12-TET/Scala/MTS-ESP); Reference pitch knob; Scala file loading; Interactive pitch circle; Editable interval list (supports any scale size); Tonic note selector; Playable 1-octave keyboard

### Formats
- VST3
- AU (Audio Unit)
- Standalone

**Last Updated:** 2026-01-25
