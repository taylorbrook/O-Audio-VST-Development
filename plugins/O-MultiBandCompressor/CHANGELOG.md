# O-MultiBandCompressor Changelog

## Version 1.2.0 (2026-01-25)

### Added

- **Real-Time FFT Spectrum Analyzer:** Live frequency visualization in the spectrum display area
  - 2048-sample FFT using `juce::dsp::FFT` with Hann windowing
  - Lock-free audio→UI communication via atomic float arrays
  - 128-bin WebView canvas visualization (downsampled from 1024 bins)
  - Logarithmic frequency scale (20 Hz - 20 kHz)
  - Smooth visual transitions (0.7 smoothing factor)
  - Gradient fill with olive/brown color scheme matching UI aesthetic
  - Real-time update at 30 Hz display rate
  - Shows actual frequency content of processed audio signal

### Technical Details

- **Audio Thread (real-time safe):**
  - Sample accumulation in ring buffer (FIFO)
  - Mono sum of L/R channels for analysis
  - FFT computed when 2048 samples accumulated
  - Magnitude conversion to normalized dB scale (-100 to 0 dB → 0 to 1)
  - Atomic store to spectrum data array

- **UI Thread:**
  - Conditional send only when new FFT data available
  - Bin averaging for 128-point display (8:1 reduction)
  - JSON array serialization for WebView transfer

- **WebView Rendering:**
  - Canvas 2D context with retina display support
  - Gradient fill from bottom (green) to top (brown)
  - Line stroke overlay for spectrum contour
  - Grid overlay at key frequencies (50, 100, 200, 500, 1k, 2k, 5k, 10k Hz)
  - Window resize handling

---

## Version 1.1.0 (2026-01-25)

### Added

- **Draggable Crossover Controls:** Click and drag crossover lines to adjust frequency split points directly in the UI
  - XOVER1 (Low/Low-Mid): 20-500 Hz
  - XOVER2 (Low-Mid/High-Mid): 200-5000 Hz
  - XOVER3 (High-Mid/High): 2000-16000 Hz
- Ordering constraints enforced during drag (XOVER1 < XOVER2 < XOVER3 with 100 Hz minimum gap)
- Visual feedback: hover highlights, drag state with green accent, enlarged handles
- Touch support for tablet/trackpad use
- Proper JUCE drag start/end notifications for undo/redo grouping

### Changed

- Crossover lines now have smooth hover transitions
- Disabled CSS transitions during drag for instant visual response

---

## Version 1.0.0 - Stage 3 Complete (2026-01-25)

**PRODUCTION READY:** All stages complete - build system, DSP processing, and GUI with real-time metering functional.

### Phase 5.3 - Real-Time Metering (2026-01-25) - FINAL PHASE

**Metering Infrastructure:**
- Timer callback at 30 Hz for smooth updates
- 4 per-band gain reduction meters (LOW, LOMID, HIMID, HIGH)
- GR meters display 0 to -24 dB compression activity
- Input/output level meters with stereo averaging
- Peak level detection using buffer.getMagnitude()
- Atomic floats for thread-safe communication (audio → UI)
- JavaScript meter update functions
- CSS transitions for smooth animations (30-50ms ease-out)

**Crossover Visualization:**
- Dynamic crossover line positioning
- Reads XOVER1/XOVER2/XOVER3 parameters in real-time
- Logarithmic frequency-to-position mapping (20 Hz - 20 kHz)
- Auto-updating frequency labels with Hz/kHz formatting

**Performance:**
- 30 Hz UI refresh rate (33ms interval)
- will-change CSS optimization for 60fps rendering
- No UI thread starvation
- Stable in DAW during playback

### Phase 5.2 - Parameter Binding (2026-01-25)

**Parameter Bindings:**
- 56 WebSliderParameterAttachment (8 global + 48 per-band)
- 13 WebToggleButtonParameterAttachment (1 global + 12 per-band)
- 1 WebComboBoxParameterAttachment (MS_MODE)
- All relays registered with .withOptionsFrom()
- Bidirectional sync: UI ↔ APVTS
- Automation support verified
- Preset save/load functional

**UI Controls:**
- Per-band knobs: Threshold, Ratio, Attack, Release, Knee, Makeup (24 total)
- Per-band buttons: Solo, Bypass, SC Listen (12 total)
- Global knobs: Input, Output, Mix (3 total)
- Global toggle: Auto-Makeup
- Global select: M/S Mode
- Value formatting: dB, ms, ratio, %

### Phase 5.1 - WebView Layout (2026-01-25)

**UI Structure:**
- WebView integration using juce::WebBrowserComponent
- Botanical/Ouaricon aesthetic (900x600px)
- HTML/CSS layout with 4-band sections
- Spectrum analyzer placeholder with grid
- Input/output meter placeholders
- Per-band GR meter placeholders
- Crossover visualization (3 handles)
- JUCE JavaScript bridge
- Resource provider for embedded assets
- Color-coded bands (brown, green, gold, orange)

### Phase 4.3 - Advanced Features (2026-01-25)

**Sidechain Filtering:**
- Per-band sidechain HPF (20-2000 Hz)
- Per-band sidechain LPF (500-20000 Hz)
- Filters applied to detector path only
- SC Listen mode for monitoring filtered signal

**Mid/Side Processing:**
- M/S encoding/decoding (power-preserving with sqrt(2))
- 4 processing modes: Off/Mid/Side/Both
- Both mode = 8 independent compressors (mid + side per band)

**Global Features:**
- Auto-makeup gain (80% compensation, 500ms smoothing)
- Dry/wet mixer (juce::dsp::DryWetMixer)
- Parallel compression capability
- All 56 parameters fully integrated

### Phase 4.2 - Crossover Network (2026-01-25)

**Multiband Architecture:**
- Linkwitz-Riley 4th order crossover (24 dB/oct)
- 3 crossover points with frequency validation
- Cascaded 2nd order Butterworth filters (12 IIR per channel)
- 4-band output: LOW, LOMID, HIMID, HIGH
- Flat magnitude summing (Linkwitz-Riley property)
- 4 independent compressor instances
- Per-band solo/bypass routing
- Gain reduction metering for all 4 bands

### Phase 4.1 - Compressor Foundation (2026-01-25)

**Compression Engine:**
- Feed-forward compressor topology
- Soft knee with quadratic interpolation (0-24 dB)
- Peak detector (absolute value)
- RMS detector (circular buffer, 10ms window)
- Peak/RMS blend (0-100% continuous)
- Envelope smoother with attack/release ballistics
- Input/output gain stages
- Bypass logic
- Stereo-linked compression
- Gain reduction metering (atomic float)

### Stage 1 - Foundation Complete (2026-01-25)

**Build System:**
- Created CMakeLists.txt with JUCE 8 configuration
- Plugin code: OMbc (4 chars)
- Manufacturer code: OuAu (Ouaricon Audio)
- Formats: VST3, AU, Standalone
- NEEDS_WEB_BROWSER TRUE for future WebView UI
- juce_dsp module added for DSP components

**Parameters (56 total):**
- 8 global parameters implemented (INPUT_GAIN, OUTPUT_GAIN, MIX, AUTO_MAKEUP, MS_MODE, XOVER1, XOVER2, XOVER3)
- 48 per-band parameters implemented (12 per band × 4 bands)
- Band prefixes: LOW, LOMID, HIMID, HIGH
- All parameters use JUCE 8 ParameterID format
- Logarithmic scaling for crossover frequencies (0.3 skew factor)
- State management (save/load) implemented

**Audio Processing:**
- PluginProcessor with pass-through audio (no DSP yet)
- Stereo input/output bus configuration
- prepareToPlay/releaseResources stubs ready for DSP
- Real-time safe parameter access example provided

**UI:**
- PluginEditor placeholder (900x600)
- Shows plugin name and parameter count
- WebView integration pending Stage 3

---

## Stage 0 - Research & Planning (2026-01-25)

**Research:**
- Professional multiband compressor examples studied (FabFilter Pro-MB, Waves C6, UAD Precision Multiband)
- JUCE modules identified for implementation
- DSP feasibility verified

**Architecture:**
- Linkwitz-Riley 4th order crossovers (24 dB/octave)
- Feed-forward compressor topology with soft knee
- Peak/RMS blend detection
- Mid/Side processing modes
- Per-band sidechain filtering
- Real-time FFT spectrum analyzer
- Auto-makeup gain with slow ballistics
- Global dry/wet mixer

**Complexity Assessment:**
- Score: 5.0 (Maximum complexity)
- 56 parameters
- 10 DSP components
- Phased implementation strategy defined

**Contracts Created:**
- architecture.md (complete DSP specification)
- plan.md (phase breakdown with test criteria)
- creative-brief.md (original concept)
- parameter-spec.md (56 parameter definitions)
