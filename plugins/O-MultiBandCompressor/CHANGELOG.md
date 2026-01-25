# O-MultiBandCompressor Changelog

## [Unreleased]

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

**Next:** Phase 4.1 - Implement single-band compressor foundation

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
