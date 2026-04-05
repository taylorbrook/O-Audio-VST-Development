# O-Bowed - Implementation Plan

**Date:** 2026-04-04
**Complexity Score:** 5.0 (Maximum -- raw score 18.0 capped at 5.0)
**Strategy:** Phase-based implementation
**Complexity Tier:** 6 (Physical modeling synth with custom DSP, multi-string, MPE, microtonal)

---

## Complexity Factors

- **Parameters:** 22 parameters (22/5 = 4.4, capped at 2.0) = **2.0**
- **Algorithms:** 9 DSP components = **9**
  1. Bow Model (custom exciter)
  2. Nonlinear Friction Junction (custom, tiered: hyperbolic/elasto-plastic/thermal)
  3. Digital Waveguide String (juce::dsp::DelayLine x2 per string)
  4. Bridge Filter (juce::dsp::IIR::Filter)
  5. Body Resonator (8x juce::dsp::IIR::Filter parallel)
  6. Sympathetic String Coupling (0-12 passive waveguides)
  7. Bow Noise Generator (custom noise + filter)
  8. 2x Oversampling (juce::dsp::Oversampling)
  9. Stereo Width Processor (custom mid-side)
- **Features:** 7 points
  - Nonlinear junction solving (+1) -- implicit equation with NR solver
  - Tiered quality modes (+1) -- 3 friction models switchable at runtime
  - Morphable body coefficients (+1) -- interpolation between preset biquad banks
  - Multi-string management (+1) -- 1-4 active + 0-12 sympathetic
  - MPE support (+1) -- per-note expression routing
  - Microtonal tuning (+1) -- Scala/TUN, MTS-ESP integration
  - Oversampling (+1) -- 2x internal for friction junction
- **Total:** 2.0 + 9 + 7 = 18.0 (capped at **5.0**)

---

## Stages

- Stage 0: Research & Planning -- COMPLETE
- Stage 1: Foundation (project structure, CMakeLists.txt, APVTS parameters)
- Stage 2: DSP -- 5 phases (see below)
- Stage 3: GUI -- 3 phases (see below)
- Stage 4: Validation (presets, pluginval, changelog)

---

## Stage 1: Foundation

**Goal:** Create project structure, CMakeLists.txt, APVTS with all 22 parameters, empty processor/editor shells

**Key Configuration:**
```cmake
IS_SYNTH TRUE
NEEDS_MIDI_INPUT TRUE
NEEDS_WEB_BROWSER TRUE
```

**BusesProperties:** Output-only (instrument, no audio input)
```cpp
AudioProcessor(BusesProperties()
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

**JUCE Modules Required:**
- `juce_audio_processors` -- core processor
- `juce_audio_basics` -- audio buffers, MIDI
- `juce_dsp` -- DelayLine, IIR::Filter, Oversampling, ProcessSpec
- `juce_gui_extra` -- WebBrowserComponent (WebView UI)
- `juce_gui_basics` -- basic UI components

**Shared Modules:**
- `modules/tuning/scala-tuning-engine` -- microtonal tuning (link via CMake)

---

## Stage 2: DSP Phases

### Phase 3.1: Core Waveguide + Basic Friction

**Goal:** Single bowed string producing sound with core friction model

**Components:**
- BowedStringVoice (extends juce::SynthesiserVoice)
- BowModel (bow velocity/force/position from MIDI)
- HyperbolicFriction (core tier -- memoryless bow table)
- WaveguideString (dual delay line with nut/bridge reflections)
- Bridge loss filter (one-pole lowpass)
- Basic output path (mono signal)
- MIDI note -> frequency -> delay length

**Parameters active:** BOW_SPEED, BOW_PRESSURE, BOW_POSITION, ROSIN, BRIGHTNESS, OUTPUT_LEVEL

**Test Criteria:**
- [ ] Plugin loads in DAW as instrument (not effect)
- [ ] MIDI note-on produces audible bowed string tone
- [ ] BOW_SPEED affects volume
- [ ] BOW_PRESSURE affects tone quality (light=airy, heavy=rich)
- [ ] BOW_POSITION changes timbre (ponticello vs tasto)
- [ ] ROSIN changes friction aggressiveness
- [ ] BRIGHTNESS controls high-frequency content
- [ ] Note-off produces natural decay
- [ ] No clicks or pops during parameter changes
- [ ] Sustained tone is stable (no runaway or silence after a few seconds)

### Phase 3.2: Body Resonator + Stereo

**Goal:** Morphable body resonator gives instrument identity, stereo output

**Components:**
- BodyResonator (8 parallel biquads with morphable coefficients)
- Preset coefficient banks: membrane, wood-violin, wood-cello, metal, glass
- Body coefficient morphing (MATERIAL interpolation)
- Body size scaling (SIZE frequency shift)
- Stereo width processor (mid-side)

**Parameters active:** +BODY_MATERIAL, BODY_SIZE, WIDTH

**Test Criteria:**
- [ ] BODY_MATERIAL morph: membrane (nasal/erhu) -> wood (violin) -> metal -> glass audibly different
- [ ] BODY_SIZE morph: small (bright) -> large (deep) frequency shift audible
- [ ] Smooth morph without clicks or level jumps
- [ ] Stereo output with WIDTH parameter working (mono at 0%, stereo at 100%, wide at 200%)
- [ ] Body resonator does not cause feedback or instability
- [ ] Material=Wood + Size=Small sounds violin-like
- [ ] Material=Membrane + Size=Medium sounds erhu-like

### Phase 3.3: Multi-String + Sympathetic Coupling

**Goal:** Multiple active strings, sympathetic resonance

**Components:**
- Multi-string management (STRING_COUNT controls 1-4 active strings)
- Per-string tuning offsets (STRING_TUNING_1-4)
- Sympathetic string engine (0-12 passive Karplus-Strong waveguides)
- Sympathetic coupling from bridge output
- String panning across stereo field

**Parameters active:** +STRING_COUNT, STRING_TUNING_1-4, SYMPATHETIC_AMOUNT, SYMPATHETIC_COUNT

**Test Criteria:**
- [ ] STRING_COUNT=1: single string works correctly
- [ ] STRING_COUNT=2: two strings at different pitches (tuning offsets)
- [ ] STRING_COUNT=4: four strings audible, CPU within budget
- [ ] Per-string tuning offsets shift pitch correctly
- [ ] Sympathetic strings add subtle resonance when SYMPATHETIC_AMOUNT > 0
- [ ] Sympathetic strings tuned chromatically respond to played notes
- [ ] CPU usage with 4 active + 12 sympathetic stays under 25% on target hardware
- [ ] Gating optimization: silent sympathetic strings don't consume significant CPU

### Phase 3.4: Advanced Friction + Impossible Physics

**Goal:** Enhanced/quality friction tiers, impossible physics parameters

**Components:**
- ElastoPlasticFriction (enhanced tier with bristle state)
- ThermalFriction (quality tier with temperature tracking)
- Newton-Raphson solver for enhanced/quality tiers
- Friction tier selector (runtime quality control)
- Infinite sustain (bridge loss reduction)
- Reversed friction (curve inversion blend)
- Sub-harmonics generator (nonlinear feedback)

**Parameters active:** +INFINITE_SUSTAIN, REVERSED_FRICTION, SUB_HARMONICS

**Test Criteria:**
- [ ] Core tier: same as before (baseline preserved)
- [ ] Enhanced tier: attack transients have more "bite" vs core
- [ ] Quality tier: sustained notes have subtle evolution vs enhanced
- [ ] Tier switching works without clicks or glitches
- [ ] Newton-Raphson converges reliably (no audible artifacts from divergence)
- [ ] INFINITE_SUSTAIN at 100%: tone sustains indefinitely
- [ ] REVERSED_FRICTION creates unusual timbres without instability
- [ ] SUB_HARMONICS adds audible sub-octave content
- [ ] No runaway or instability with extreme "impossible physics" settings

### Phase 3.5: Oversampling + Tuning + Optimization

**Goal:** 2x oversampling for aliasing reduction, microtonal tuning integration, performance optimization

**Components:**
- juce::dsp::Oversampling<float> wrapping friction/waveguide section
- Tuning engine integration (Scala/TUN, MTS-ESP, 12TET)
- Reference pitch parameter
- Bow noise generator
- MPE routing (aftertouch, slide, pitch bend per-note)
- CPU profiling and optimization pass

**Parameters active:** +REFERENCE_PITCH, TUNING_SYSTEM (all 22 parameters now active)

**Test Criteria:**
- [ ] Oversampling reduces aliasing (A/B comparison with and without)
- [ ] Latency correctly reported to host
- [ ] Scala file loading works (test pythagorean, just intonation)
- [ ] 12TET default produces standard tuning
- [ ] REFERENCE_PITCH shifts all pitches correctly
- [ ] MPE aftertouch controls bow pressure per-note
- [ ] MPE slide controls bow position per-note
- [ ] MPE pitch bend produces smooth portamento
- [ ] Bow noise adds subtle texture without dominating
- [ ] CPU targets met: <6% for 2 strings core tier, <25% for max configuration
- [ ] No denormals (ScopedNoDenormals in processBlock)

---

## Stage 3: GUI Phases

### Phase 4.1: Layout and Basic Controls

**Goal:** WebView UI with parameter knobs and layout

**Components:**
- WebView setup (index.html + CSS + JS)
- Parameter binding via WebSliderRelay / WebSliderParameterAttachment
- Knob controls for: BOW_SPEED, BOW_PRESSURE, BOW_POSITION, ROSIN
- Knob controls for: BODY_MATERIAL, BODY_SIZE, BRIGHTNESS
- Output level and width controls
- Section grouping (Bow, Body, Strings, Output, Impossible, Tuning)

**Test Criteria:**
- [ ] WebView opens with correct size
- [ ] All knobs visible and styled
- [ ] Knobs respond to mouse drag (relative drag pattern)
- [ ] Layout matches design sections
- [ ] Background and styling render properly

### Phase 4.2: Parameter Binding and Interaction

**Goal:** Full two-way parameter communication

**Components:**
- JavaScript -> C++ relay calls (all 22 parameters)
- C++ -> JavaScript parameter updates (host automation)
- Integer parameters: STRING_COUNT, SYMPATHETIC_COUNT
- Choice parameter: TUNING_SYSTEM
- Value formatting and display
- Preset load/save updates all UI elements

**Test Criteria:**
- [ ] All 22 parameters controllable from UI
- [ ] Host automation updates UI controls in real-time
- [ ] Preset changes update all UI elements
- [ ] INTEGER parameters (STRING_COUNT) show discrete values
- [ ] CHOICE parameter (TUNING_SYSTEM) shows dropdown/selector
- [ ] No lag or visual glitches during parameter changes

### Phase 4.3: Advanced UI Elements

**Goal:** Visualization and advanced UI features

**Components:**
- Bow-string visualization (animated bow contact point)
- Body resonance spectrum display (frequency response of current body preset)
- Schelleng diagram visualization (show playable region)
- Friction tier selector
- Tuning system file browser (Scala/TUN file loading)
- Preset browser
- String tuning display (per-string pitch visualization)

**Test Criteria:**
- [ ] Bow-string visualization animates in response to MIDI input
- [ ] Body spectrum display shows resonance peaks
- [ ] Schelleng diagram shows current playing position
- [ ] Friction tier selector changes quality mode
- [ ] File browser loads Scala/TUN files
- [ ] Preset browser navigates and loads presets
- [ ] Performance acceptable (no CPU spikes from UI updates)

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- Body resonator coefficients: double-buffer with atomic pointer swap
- Friction tier: `std::atomic<int>` (0=core, 1=enhanced, 2=quality)
- Tuning table: atomic pointer swap from message thread
- Bristle displacement (z) and temperature (T_contact) are per-voice -- no cross-thread access
- Voice allocation via juce::Synthesiser handles locking internally

### Performance
- Core friction + waveguide per string: ~2% CPU (44.1 kHz)
- Enhanced friction: ~3x core
- Quality friction: ~5x core
- Body resonator (8 biquads): ~0.5%
- Sympathetic string: ~0.1-0.3% each
- 2x oversampling: ~2.2x multiplier on friction/waveguide
- Budget: single string core = ~4.4% (with 2x OS), two strings = ~9%, four strings = ~18%

### Latency
- Oversampling: ~8 samples (halfband filter group delay)
- Waveguide: 0 algorithmic latency (causal)
- Report via `setLatencySamples()` in `prepareToPlay()`

### Denormal Protection
- Use `juce::ScopedNoDenormals` in processBlock()
- juce::dsp::DelayLine and juce::dsp::IIR::Filter handle denormals internally
- Clamp bristle displacement z and temperature T_contact to prevent tiny values

### Known Challenges
- Newton-Raphson divergence: clamp iterations to 6, bail out with previous solution
- Elasto-plastic passivity: apply velocity-dependent damping fix from recent research
- Body coefficient morphing: use makePeakFilter() recalculation (safer than raw coefficient lerp)
- Multi-string CPU scaling: profile early, implement gating for sympathetic strings
- Reference implementations: STK Bowed.cpp, O-Lyrica WaveguideString, FAUST physmodels.lib

---

## References

- Creative brief: `plugins/O-Bowed/.planning/BRIEF.md`
- Parameter spec: `plugins/O-Bowed/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-Bowed/.planning/research/ARCHITECTURE.md`
- Requirements: `plugins/O-Bowed/.planning/REQUIREMENTS.md`

### Research Documents
- `research/O-Bowed-research-synthesis.md` -- Unified architecture from 3 research agents
- `research/bow-string-friction-models.md` -- 4 friction models with equations and C++
- `research/O-Bowed-market-research.md` -- Competitive analysis
- `research/O-Bowed-acoustic-instrument-research.md` -- Instrument acoustics

### Reference Plugins in Codebase
- O-Lyrica -- WaveguideString, HarpSynthVoice pattern, SympatheticResonance, TuningEngine integration
- O-Prism -- TuningEngine shared module usage, complex synth ROADMAP pattern
- O-Bells -- Physical modeling synth voice management
