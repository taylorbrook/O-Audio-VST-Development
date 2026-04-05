# O-Formant - Implementation Plan

**Date:** 2026-04-04
**Complexity Score:** 5.0 (raw: 8.2) (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 21 parameters (21/5 = 4.2, capped at 2.0) = **2.0**
- **Algorithms:** 5 DSP components = **5**
  - LF Glottal Pulse Model (custom, Newton-Raphson solvers, PolyBLEP)
  - Formant Filter Bank (5 custom biquad BPFs, parallel topology)
  - Vowel Interpolation Engine (Shepard, log-domain, 5 anchors)
  - Consonant Noise Branch (KLATT dual-branch, tone/sibilance/burst)
  - Vibrato LFO (sine, delay ramp, per-voice)
- **Features:** 1.2 points
  - Modulation systems: +1 (vibrato LFO with delay ramp)
  - External MIDI control (MPE): +0.2 (MPESynthesiser handles protocol; minimal custom code)
- **Total raw:** 2.0 + 5.0 + 1.2 = **8.2** (capped at **5.0**)

**Note:** Raw score of 8.2 reflects significant implementation weight from novel DSP algorithms (LF model, formant bank, consonant system). The 5.0 cap understates actual complexity. This is comparable to O-Prism (raw 14.0) in DSP novelty, but simpler in total feature count.

---

## Complexity Tier

**Tier 4: Synthesizer with MIDI input, oscillators**
- Polyphonic synthesizer with MPE
- Novel DSP algorithms (LF model, formant interpolation)
- Custom per-voice processing chain
- Research depth: MODERATE-DEEP

---

## Stages

- Stage 0: Research & Planning -- COMPLETE
- Stage 1: Foundation (build system, APVTS, processor/editor skeleton)
- Stage 2: DSP (phased -- 3 phases)
- Stage 3: GUI (phased -- 3 phases)
- Stage 4: Polish (presets, validation, changelog)

---

## Complex Implementation (Score >= 3.0)

### Stage 1: Foundation

**Goal:** Build system, APVTS with all 21 parameters, MPESynthesiser skeleton with silent voices

**Components:**
- CMakeLists.txt with IS_SYNTH TRUE, NEEDS_MIDI_INPUT TRUE, correct module dependencies
- PluginProcessor with MPESynthesiser, enableLegacyMode(), 16 voices
- FormantVoice skeleton (extends MPESynthesiserVoice) -- outputs silence
- APVTS with all 21 parameters (correct ranges, defaults, skews)
- PluginEditor placeholder (generic or WebView scaffold)
- BusesProperties: output-only stereo

**Test Criteria:**
- [ ] Plugin builds (VST3 + AU)
- [ ] Plugin loads in DAW as instrument
- [ ] MIDI notes trigger voice allocation (verify via debug log)
- [ ] All 21 parameters visible in DAW automation
- [ ] pluginval passes basic scan

---

### Stage 2: DSP Phases

#### Phase 2.1: Core Vocal Engine (Glottal + Formants + ADSR)

**Goal:** Playable basic vocal synth -- LF glottal source through formant filter bank with envelope

**Components:**
- LFGlottalSource: Rd parameter, Fant 1995 regression, Newton-Raphson solvers, PolyBLEP
- FormantBiquad struct: Custom biquad with makeBandPass coefficient computation
- FormantFilterBank: 5 parallel BPFs with hardcoded vowel "A" (or center position)
- VoiceEnvelope: juce::ADSR integration
- AspirationNoise: White noise mixer with breathiness control
- Basic VowelMorpher: Shepard interpolation from XY parameters

**Test Criteria:**
- [ ] Playing MIDI notes produces voiced "aah" sound (formant A)
- [ ] glottalRd parameter audibly changes voice quality (pressed to breathy)
- [ ] breathiness parameter adds noise to glottal source
- [ ] ADSR envelope shapes amplitude correctly (verify attack/release)
- [ ] vowelX/Y parameters morph between vowels (hear I/E/A/O/U)
- [ ] vowelFocus parameter changes interpolation sharpness
- [ ] formantShift shifts gender (male to female)
- [ ] No clicks, pops, or NaN at any parameter setting
- [ ] Full MIDI range (C0-C8) produces output without aliasing artifacts below C5
- [ ] 16-voice polyphony works without voice stealing artifacts

#### Phase 2.2: Modulation and Expression

**Goal:** Add vibrato, portamento, MPE expression, and consonant noise

**Components:**
- VibratoLFO: Sine LFO with rate, depth, onset delay
- PitchGlide: Exponential portamento smoother
- MPE integration: Pressure -> breathiness, Slide -> vowelY offset, Velocity -> attack character
- ConsonantEngine: Noise branch with tone control, sibilance peak
- Auto-consonant plosive burst generator

**Test Criteria:**
- [ ] Vibrato audible and controllable (rate, depth, delay onset)
- [ ] Pitch glide smoothly transitions between notes (legato playing)
- [ ] MPE pressure modulates breathiness per-note
- [ ] MPE slide modulates vowel Y-axis per-note
- [ ] Velocity affects attack intensity
- [ ] Consonant noise audible and tonally controlled
- [ ] Sibilance adds high-frequency emphasis
- [ ] Auto-consonant produces natural plosive burst on note-on
- [ ] All features work together without artifacts

#### Phase 2.3: Output Stage and Polish

**Goal:** Stereo spread, output gain, performance optimization

**Components:**
- StereoSpread: Per-voice panning by pitch
- OutputGain: juce::dsp::Gain with dB control
- Performance optimization: Skip consonant branch when level=0, early-out for silent voices
- Parameter smoothing verification: No zipper noise on any parameter

**Test Criteria:**
- [ ] Stereo width parameter spreads voices across stereo field
- [ ] Output gain controls level without distortion
- [ ] CPU usage at 16 voices < 5% single core at 48kHz
- [ ] No zipper noise when automating any parameter
- [ ] ScopedNoDenormals prevents denormal buildup
- [ ] Filter states properly reset on voice release

---

### Stage 3: GUI Phases

#### Phase 3.1: Layout and Basic Controls

**Goal:** WebView UI with XY vowel morph pad and parameter groups

**Components:**
- HTML/CSS layout: Two-column (XY pad left, parameter groups right, ADSR + output bottom)
- XY pad: Draggable cursor with 5 vowel labels at acoustic positions
- Basic knob controls for all parameter groups
- WebView setup with resource provider

**Test Criteria:**
- [ ] WebView window opens with correct size
- [ ] XY pad renders with vowel labels at correct positions
- [ ] Cursor is draggable on XY pad
- [ ] All knobs visible and styled in correct groups
- [ ] Layout matches BRIEF.md UI concept

#### Phase 3.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI <-> DSP)

**Components:**
- WebSliderRelay + WebSliderParameterAttachment for all 21 parameters
- XY pad -> vowelX/vowelY binding (custom relay for 2D control)
- Host automation updates UI controls
- Value display formatting (Hz, dB, ms, etc.)
- autoConsonant toggle button binding

**Test Criteria:**
- [ ] All knob movements change DSP parameters in real-time
- [ ] Host automation updates UI controls
- [ ] XY pad drag updates vowelX and vowelY simultaneously
- [ ] Preset changes update all UI elements
- [ ] Parameter values display correctly with units
- [ ] No lag between UI interaction and audio change

#### Phase 3.3: Advanced UI Elements

**Goal:** Formant peaks overlay, cursor glow, visual polish

**Components:**
- Formant peaks overlay on XY pad (F1-F5 real-time frequency bars or markers)
- Cursor trailing glow effect
- ADSR visual display (curve visualization)
- Color scheme and typography polish

**Test Criteria:**
- [ ] Formant peaks update in real-time as XY position changes
- [ ] Cursor glow renders smoothly
- [ ] ADSR display matches parameter values
- [ ] Visual polish consistent across all controls
- [ ] Performance acceptable (no UI-induced CPU spikes)

---

### Stage 4: Polish

**Goal:** Presets, validation, documentation

**Components:**
- Factory presets (16+): Cinematic, Electronic, Ambient, Speech categories
- pluginval validation
- CHANGELOG.md
- Build and install to system folders

**Test Criteria:**
- [ ] All presets load without crashes
- [ ] Presets demonstrate range of sonic capabilities
- [ ] pluginval passes all tests
- [ ] Plugin persists state across DAW sessions
- [ ] Installed successfully to VST3 + AU system folders

---

## Implementation Flow

- Stage 0: Research & Planning -- COMPLETE
- Stage 1: Foundation (build, APVTS, MPESynthesiser skeleton)
- Stage 2: DSP -- 3 phases
  - Phase 2.1: Core Vocal Engine (glottal + formants + vowel morph + ADSR)
  - Phase 2.2: Modulation and Expression (vibrato, glide, MPE, consonants)
  - Phase 2.3: Output Stage and Polish (stereo, gain, optimization)
- Stage 3: GUI -- 3 phases
  - Phase 3.1: Layout and Basic Controls
  - Phase 3.2: Parameter Binding and Interaction
  - Phase 3.3: Advanced UI Elements
- Stage 4: Polish (presets, pluginval, changelog)

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- Per-voice state is private (no shared mutable state between voices)
- Each voice has its own `juce::Random` instance
- SmoothedValue instances are per-component, advanced in audio thread only
- No locks anywhere in audio path

### Performance
- Per-voice: ~95-100 FLOPS/sample (LF: 20, Filters: 50, Noise: 15, Envelope: 15)
- 16 voices at 48kHz: ~77 MFLOPS = ~1.5% single core
- Block-rate coefficient updates (every 32 samples) amortize Shepard interpolation + trig
- Early-out: skip consonant branch when `consonantLevel == 0`
- Hot path: glottal eval + 5 biquad process + ADSR multiply

### Latency
- Zero latency (no lookahead, no oversampling, no FFT)
- `setLatencySamples(0)` in prepareToPlay

### Denormal Protection
- `juce::ScopedNoDenormals` in processBlock()
- Biquad z-states reset in `clearCurrentNote()` (voice release)
- ADSR returns exactly 0.0 after release

### Known Challenges
- LF model Newton-Raphson convergence at extreme Rd values -- add iteration cap + NaN guard
- Biquad coefficient stability at very high formant frequencies near Nyquist -- clamp to sampleRate/2 - 100
- PolyBLEP placement requires accurate detection of phase crossing Te -- use sub-sample detection
- XY pad to dual-parameter binding: WebView relay system needs custom handling for 2D control
- Plosive burst crossfade with ADSR attack: timing must feel natural (10-25ms burst, smooth transition)
- O-Prism pattern reference: MPESynthesiserVoice with APVTS pointer passed to each voice

---

## References

- Creative brief: `plugins/O-Formant/.planning/BRIEF.md`
- Parameter spec: `plugins/O-Formant/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-Formant/.planning/research/ARCHITECTURE.md`
- Requirements: `plugins/O-Formant/.planning/REQUIREMENTS.md`

**Research documents:**
- `research/O-Formant-deep-research.md` -- Master synthesis research
- `research/2d-vowel-morph-xy-pad.md` -- XY pad geometry and interpolation
- `research/consonant-noise-synthesis.md` -- Consonant system design
- `research/glottal-pulse-modeling-deep-dive.md` -- LF model implementation detail

**Reference plugins:**
- O-Prism -- MPESynthesiser pattern, APVTS-per-voice, custom oscillator approach
- O-Lyrica -- SynthesiserVoice pattern with TuningEngine, proven voice architecture
- O-Bells -- Physical model synth pattern
