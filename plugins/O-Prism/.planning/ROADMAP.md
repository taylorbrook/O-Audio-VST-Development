# O-Prism - Implementation Plan

**Date:** 2026-02-16
**Complexity Score:** 5.0 (Very High, Capped)
**Strategy:** Phase-based implementation (staged DSP and GUI)

---

## Complexity Factors

- **Parameters:** 68 parameters (68/5 = 13.6, capped at 2.0) = **2.0**
- **Algorithms:** 8 DSP components = **8**
  - WavetableOscillator (custom, mipmap engine)
  - SubOscillator (polyBLEP)
  - NoiseGenerator (6 noise types)
  - SVFFilter (dual, multi-mode, with routing)
  - ADSREnvelope (x2, amp + filter)
  - UnisonEngine (1-8 voices per osc)
  - EffectsChain (5 effects: Reverb, Delay, Chorus, Distortion, EQ)
  - WavetableManager (loading, mipmap generation, memory management)
- **Features:** 4 points
  - Modulation system (filter envelope -> cutoff) (+1)
  - MIDI synthesis (SynthesiserVoice, voice stealing, glide) (+1)
  - File I/O (wavetable import WAV/AIFF, Scala/KBM import) (+1)
  - Real-time visualization (wavetable display in WebView) (+1)
- **Raw Total:** 2.0 + 8 + 4 = **14.0**
- **Final Score:** min(14.0, 5.0) = **5.0** (capped at maximum)

**Assessment:** This is the most complex plugin in the Ouaricon catalog. The raw score of 14.0 (nearly 3x the cap) reflects the breadth of novel DSP, high parameter count, and multiple system integrations. Every stage requires careful phasing.

---

## Stages

- Stage 0: Research and Planning -- COMPLETE
- Stage 1: Foundation (build system + APVTS) -- Next
- Stage 2: DSP -- 5 phases (see below)
- Stage 3: GUI -- 3 phases (see below)
- Stage 4: Validation -- presets, testing, changelog

---

## Stage 1: Foundation

**Goal:** Project scaffolding, CMakeLists.txt, APVTS with all 68 parameters, basic PluginProcessor/Editor shells

**Components:**
- CMakeLists.txt with all required JUCE modules and flags
  - `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`
  - All 7 JUCE module dependencies linked
  - `juce_generate_juce_header()` (JUCE 8 requirement)
  - Windows WebView2 static linking flag
- PluginProcessor.h/cpp with:
  - `juce::Synthesiser` member
  - `TuningEngine` member (from scala-tuning-engine module)
  - APVTS with all 68 parameters
  - Output-only BusesProperties (synth, no audio input)
  - Basic `processBlock()` that renders synthesiser
  - `getStateInformation()` / `setStateInformation()` stubs
- PluginEditor.h/cpp with:
  - WebView shell (placeholder HTML)
  - `setSize(1200, 800)`
- Copy tuning engine source files from shared module
- Stub PrismVoice and PrismSound classes

**Estimated effort:** Medium (68 parameters is significant APVTS work)

**Test Criteria:**
- [ ] Project builds with no errors (VST3 + AU + Standalone)
- [ ] Plugin loads in DAW as instrument (appears in synth/instrument category)
- [ ] DAW routes MIDI to plugin (verify with MIDI monitor)
- [ ] All 68 parameters visible in DAW automation list
- [ ] Plugin produces silence (no DSP yet, but no crashes)

---

## Stage 2: DSP Phases

### Phase 2.1: Basic Wavetable Playback

**Goal:** Single wavetable oscillator playing notes with basic ADSR -- the minimum viable synthesizer

**Components:**
- `WavetableOscillator` class: Phase accumulator, linear interpolation from raw wavetable (no mipmap yet)
- `PrismVoice`: Single oscillator (Osc A only) rendering into voice buffer
- `PrismSound`: Minimal sound class (responds to all notes/channels)
- Amplitude ADSR: Applied to voice output
- Basic wavetable data: Generate simple waveforms (saw, square, sine) as test tables at startup
- TuningEngine integration: `getFrequency(midiNote)` for pitch calculation
- Coarse/fine tuning applied to frequency

**Test Criteria:**
- [ ] Play MIDI notes and hear sound
- [ ] Different wavetable positions produce different timbres
- [ ] Amplitude envelope shapes notes correctly (attack/decay/sustain/release)
- [ ] Tuning is accurate (A4 = 440 Hz verified)
- [ ] Polyphony works (play chords, hear multiple voices)
- [ ] Voice stealing works (exceed 16 voices, oldest note released)
- [ ] No clicks on note-on or note-off

---

### Phase 2.2: Mipmap Anti-Aliasing + Osc B + Mixing

**Goal:** Bandlimited wavetable playback, both oscillators active, oscillator mixing

**Components:**
- Mipmap generation using `juce::dsp::FFT`:
  - 10 octave levels per frame
  - Float storage for memory efficiency
  - Generate at table load time
- Mipmap level selection during playback
- Interpolation between mipmap levels for smooth transitions
- Oscillator B activation (identical architecture to A)
- Oscillator mixer: `oscMix` parameter crossfade between A and B
- Per-oscillator level and pan controls

**Test Criteria:**
- [ ] No audible aliasing when playing high notes (>C6)
- [ ] Smooth mipmap transitions when pitch changes
- [ ] Both oscillators active simultaneously
- [ ] oscMix sweeps smoothly between A and B
- [ ] Level and pan controls work per oscillator
- [ ] CPU usage reasonable (benchmark: <5% per voice with both osc)

---

### Phase 2.3: Unison + Sub + Noise + Glide

**Goal:** Full oscillator section complete -- unison spreading, sub oscillator, noise, portamento

**Components:**
- `UnisonEngine`: 1-8 detuned voices per oscillator
  - Linear detune spread
  - Stereo width panning
  - Amplitude normalization (1/sqrt(N))
  - Random phase offset per voice
- `SubOscillator`: polyBLEP anti-aliased classic waveforms
  - Sine, Triangle, Saw, Square
  - Octave offset (-2 to 0)
  - Direct to output (bypass filters)
- `NoiseGenerator`: 6 noise types
  - White, Pink (Voss-McCartney), Brown (leaky integrator)
  - Digital (sample-and-hold), Vinyl (filtered + crackle), Wind (modulated LP brown)
- Glide/portamento processor:
  - Exponential frequency interpolation
  - Off/Legato/Always modes

**Test Criteria:**
- [ ] Unison 8 voices produces thick, wide sound
- [ ] Unison detune spreads pitch symmetrically
- [ ] Unison width creates stereo image
- [ ] Sub oscillator sounds at correct octave below fundamental
- [ ] Sub shapes (sine/tri/saw/square) are clean and alias-free
- [ ] All 6 noise types sound distinct and correct
- [ ] Glide creates smooth pitch transitions between notes
- [ ] CPU still within budget at max unison (8 voices x 16 polyphony)

---

### Phase 2.4: Dual Filters + Filter Envelope

**Goal:** Dual SVF multi-mode filters with serial/parallel routing and filter envelope modulation

**Components:**
- `SVFFilter` wrapper around `juce::dsp::StateVariableTPTFilter<double>`:
  - All 7 types: LP12, LP24, HP12, HP24, BP12, BP24, Notch
  - 24dB modes: Two cascaded SVF instances
  - Notch: Sum of LP + HP outputs
- Pre-filter drive: `tanh()` waveshaping
- Key tracking: Cutoff offset based on MIDI note
- Filter envelope: ADSR modulating cutoff with bipolar depth
- Filter routing: Serial (A->B) and Parallel (A+B) modes
- Per-sample cutoff modulation (envelope + key tracking applied each sample)

**Test Criteria:**
- [ ] All 7 filter types produce expected frequency response
- [ ] LP24 sounds distinctly different from LP12 (steeper rolloff)
- [ ] Resonance self-oscillates at maximum for LP/HP types
- [ ] Drive adds harmonics without distorting the filter character
- [ ] Key tracking shifts cutoff with pitch (verify C3 vs C5)
- [ ] Filter envelope sweeps cutoff smoothly (no clicks or zipper noise)
- [ ] Serial routing: A->B clearly compounds filtering
- [ ] Parallel routing: A+B sum sounds different from serial
- [ ] No filter instability or blowup at extreme settings

---

### Phase 2.5: Effects Chain + Master

**Goal:** Global effects processing -- the final DSP stage

**Components:**
- `DistortionProcessor`:
  - 4 algorithms: soft clip, hard clip, tube, fold
  - 2x oversampling via `juce::dsp::Oversampling<double>`
  - Dry/wet mix via `juce::dsp::DryWetMixer<double>`
- `juce::dsp::Chorus<double>`:
  - Rate, depth, mix controls
  - Centre delay at 7ms
- `DelayProcessor`:
  - `juce::dsp::DelayLine<double, Lagrange3rd>`
  - Feedback with LP filter (8kHz)
  - Ping-pong mode (L/R alternation)
  - Sync mode (BPM-quantized time from playHead)
  - Dry/wet mix
- `ReverbProcessor`:
  - `juce::dsp::Reverb` with pre-delay via `juce::dsp::DelayLine`
  - Size, damping, mix controls
- `EQProcessor`:
  - 3 bands: low shelf (200Hz), mid peak (variable), high shelf (8kHz)
  - `juce::dsp::IIR::Coefficients<double>` for filter design
- Master volume control

**Test Criteria:**
- [ ] All 4 distortion types sound distinct and musical
- [ ] Distortion has no aliasing (2x oversampling working)
- [ ] Chorus creates audible movement and width
- [ ] Delay time is accurate (measure with click track)
- [ ] Delay ping-pong alternates between channels
- [ ] Delay feedback doesn't run away (capped at 0.95)
- [ ] Reverb creates convincing spatial effect
- [ ] Reverb pre-delay adds separation before reverb onset
- [ ] EQ bands boost/cut at expected frequencies
- [ ] Master volume scales output correctly
- [ ] All effects at mix=0.0 pass signal unchanged (dry path)
- [ ] Effects chain order sounds musical

---

## Stage 3: GUI Phases

### Phase 3.1: Layout and Basic Controls

**Goal:** WebView UI with all sections laid out and basic parameter binding

**Components:**
- HTML/CSS layout matching BRIEF.md wireframe (1200x800)
- Ouaricon Naturalist aesthetic adaptation
- All sections: Osc A, Osc B, Sub, Noise, Osc Mix, Filter A, Filter B, Amp Env, Filter Env, Effects, Tuning, Master
- Knob components for all continuous parameters
- Dropdown menus for choice parameters (filter type, noise type, etc.)
- WebSliderRelay + WebSliderParameterAttachment for all 68 parameters
- Resource provider with explicit URL mapping (juce8-critical-patterns.md #8)

**Test Criteria:**
- [ ] WebView opens at 1200x800
- [ ] All sections visible and properly laid out
- [ ] Ouaricon Naturalist styling applied (earth tones, Garamond headers)
- [ ] Basic knob interaction works (drag to change value)
- [ ] Parameter changes from UI reach DSP (verify by ear)
- [ ] DAW automation updates UI controls

---

### Phase 3.2: Full Parameter Binding + Tuning Panel

**Goal:** Complete two-way parameter binding, tuning module UI integration

**Components:**
- All 68 parameters bound with proper WebSliderParameterAttachment (3 params, juce8-critical-patterns.md #12)
- Value display for each parameter
- Tuning panel integration from scala-tuning-engine module
  - Copy `tuning-panel.js` and CSS
  - Register all native functions (20+ from integration checklist)
  - Tuning preset selector, tonic selector, master tune
  - Scala/KBM file loading buttons
- Host automation -> UI sync
- Preset change -> UI sync

**Test Criteria:**
- [ ] All 68 parameters update DSP when adjusted in UI
- [ ] All parameters update UI when automated from DAW
- [ ] Tuning panel displays correctly
- [ ] Tuning presets change pitch audibly
- [ ] Scala file import works from UI
- [ ] Values display correctly (Hz, dB, ms, %)
- [ ] No lag between UI interaction and audio change

---

### Phase 3.3: Wavetable Visualization + Polish

**Goal:** Wavetable display, advanced UI elements, final polish

**Components:**
- Wavetable visualization (per oscillator):
  - Current waveform display (2D, Canvas)
  - Position indicator showing active frame
  - Update on position parameter change
  - 3D wireframe view (if performance allows, WebGL)
- Wavetable selector UI (dropdown or grid)
- Effect section tabbed interface (show one effect at a time)
- Botanical overlay image
- Button states, hover effects, visual feedback
- VU meter or level indicator (optional for v1.0)

**Test Criteria:**
- [ ] Wavetable waveform renders correctly for current position
- [ ] Waveform updates when position parameter changes
- [ ] Wavetable selector shows available tables
- [ ] Effect tabs switch between effect parameter views
- [ ] Botanical overlay renders at correct opacity
- [ ] UI maintains 60fps during playback
- [ ] All visual elements match Ouaricon Naturalist aesthetic
- [ ] Plugin looks professional at 1200x800

---

## Stage 4: Validation

**Goal:** Final testing, preset creation, release preparation

**Components:**
- Factory preset creation (categories: Leads, Pads, Bass, Keys, FX, Microtonal)
- pluginval testing (automated validation)
- Cross-DAW testing (Ableton, Logic, standalone)
- CPU profiling and optimization
- CHANGELOG.md creation
- Version bump to v1.0.0

**Test Criteria:**
- [ ] pluginval passes all tests
- [ ] Plugin works in Ableton Live (VST3)
- [ ] Plugin works in Logic Pro (AU)
- [ ] Plugin works in Standalone
- [ ] Factory presets sound good and showcase features
- [ ] Microtonal presets demonstrate tuning capabilities
- [ ] CPU under 25% at 16 voices with unison on modern hardware
- [ ] State save/restore works across DAW sessions
- [ ] No memory leaks (monitor over extended session)

---

## Implementation Flow Summary

```
Stage 1: Foundation
  |-- CMakeLists.txt, APVTS (68 params), processor/editor shells
  |-- Tuning engine integration
  |
Stage 2: DSP (5 phases)
  |-- Phase 2.1: Basic wavetable playback + ADSR
  |-- Phase 2.2: Mipmap anti-aliasing + Osc B + mixer
  |-- Phase 2.3: Unison + sub + noise + glide
  |-- Phase 2.4: Dual filters + filter envelope
  |-- Phase 2.5: Effects chain + master
  |
Stage 3: GUI (3 phases)
  |-- Phase 3.1: Layout + basic controls
  |-- Phase 3.2: Full binding + tuning panel
  |-- Phase 3.3: Visualization + polish
  |
Stage 4: Validation
  |-- Presets, testing, changelog, release
```

**Each DSP phase gets a git commit. Each GUI phase gets a git commit.**

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- Wavetable pointer swap via `std::atomic<WavetableData*>` for background loading
- TuningEngine atomic frequency table (128 atomic doubles, no locks)
- No mutexes on audio thread (lock-free requirement)
- JUCE Synthesiser handles voice lifecycle thread-safely

### Performance
- **Target:** <25% CPU at 16 voices with unison on modern hardware
- **Worst case estimate:** ~50% single core (16 voices x 8 unison x 2 osc)
- **Typical case:** ~15% single core (8 voices x 2 unison x 1 osc)
- **Optimization path:**
  - Float storage for mipmap data (halves memory bandwidth)
  - SIMD for wavetable interpolation (process 4 samples at once)
  - Dynamic voice limiting based on unison count
  - Profile-guided optimization after Phase 2.3

### Latency
- Zero latency (no lookahead processing)
- Distortion oversampling latency compensated internally
- `setLatencySamples(0)` in `prepareToPlay()`
- Note: `getLatencySamples()` is NOT virtual in JUCE 8

### Denormal Protection
- `juce::ScopedNoDenormals` at top of `processBlock()`
- JUCE DSP components handle denormals internally
- Custom oscillators use phase wrapping (0.0-1.0)

### Known Challenges
1. **Memory for wavetable mipmaps:** ~20MB per loaded table; need lazy loading strategy
2. **68 WebSliderRelay instances:** Highest count in catalog; use WebViewRelayManager module
3. **CPU at max unison:** 256 oscillator instances; may need dynamic voice limiting
4. **Wavetable frame interpolation + mipmap interpolation:** Two interpolation dimensions simultaneously; needs careful implementation to avoid artifacts
5. **Filter envelope per-sample modulation:** Must call `setCutoffFrequency()` every sample for smooth modulation; JUCE SVF designed for this (TPT)
6. **Factory wavetable generation:** Need 100+ wavetables before v1.0; can start with procedurally generated basic waveforms

---

## Dependencies

### Existing Code to Reuse
- `modules/tuning/scala-tuning-engine/` v2.1.0 -- Complete microtonal engine
- `modules/core/webview-relay-manager/` -- WebView relay management for 68 parameters
- O-Lyrica `HarpSynthVoice` pattern -- Voice architecture reference (APVTS pointer, TuningEngine pointer)
- O-Lyrica `PluginProcessor` pattern -- Synthesiser setup, state persistence

### External Dependencies
- JUCE 8.0.4 (local at /Users/taylorbrook/JUCE)
- WebView2 (Windows, for cross-platform support)
- Factory wavetable data (to be generated/sourced before Stage 2)

---

## Critical Path

The critical path through implementation is:

1. **Stage 1 Foundation** (APVTS is blocking for everything else)
2. **Phase 2.1 Basic Playback** (core wavetable engine, blocking for all other DSP)
3. **Phase 2.2 Mipmap** (audio quality gate, blocking for release)
4. **Phase 2.4 Filters** (essential for sound design, blocking for presets)
5. **Phase 2.5 Effects** (completes DSP, blocking for GUI integration)
6. **Phase 3.1 Layout** (blocking for parameter binding)
7. **Phase 3.2 Binding** (blocking for user testing)

Phases 2.3 (unison/sub/noise) and 3.3 (visualization) are important but not on the critical path -- they can be deferred if timeline pressure exists.

---

## References

- Creative brief: `plugins/O-Prism/.planning/BRIEF.md`
- Requirements: `plugins/O-Prism/.planning/REQUIREMENTS.md`
- DSP architecture: `plugins/O-Prism/.planning/research/ARCHITECTURE.md`
- Tuning module: `modules/tuning/scala-tuning-engine/`
- Tuning integration checklist: `modules/tuning/scala-tuning-engine/snippets/INTEGRATION-CHECKLIST.md`
- WebView relay module: `modules/core/webview-relay-manager/`
- O-Lyrica (reference synth): `plugins/O-Lyrica/Source/`
- O-Bells (reference synth): `plugins/O-Bells/Source/`
