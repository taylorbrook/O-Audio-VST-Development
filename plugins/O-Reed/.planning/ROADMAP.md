# O-Reed - Implementation Plan

**Date:** 2026-04-04
**Complexity Score:** 5.0 (Maximum -- raw score 16.4 capped at 5.0)
**Strategy:** Phase-based implementation
**Complexity Tier:** 6 (Physical modeling synth with custom DSP, Guillemain nonlinearity, bore morphing, MPE, microtonal)

---

## Complexity Factors

- **Parameters:** 35 parameters (35/5 = 7.0, capped at 2.0) = **2.0**
- **Algorithms:** 9.4 DSP components = **9.4**
  1. Reed Model -- mass-spring-damper ODE (custom)
  2. Nonlinear Junction -- Bernoulli + Guillemain Psi (custom)
  3. Bore Waveguide -- bidirectional delay lines with taper (juce::dsp::DelayLine x2)
  4. Viscothermal Loss Filter (juce::dsp::IIR::Filter)
  5. Bell Radiation Filter (juce::dsp::IIR::Filter)
  6. Tone Hole Lattice -- 4 three-port scattering junctions (custom)
  7. Mouthpiece Volume -- Helmholtz compliance (custom)
  8. Breath Noise Generator (custom noise + juce::dsp::IIR::Filter)
  9. 2x/4x Oversampling (juce::dsp::Oversampling)
  9.4. Instrument Morph System, Vibrato, Growl, Flutter, Subtone (0.4 -- parameter modifiers, not full DSP components)
- **Features:** 5 points
  - Nonlinear junction solving (+1) -- implicit reed-bore coupling with polynomial/NR
  - Conical bore correction filter (+1) -- spectral modification for bore taper
  - Dual bore system (+1) -- second parallel waveguide
  - MPE support (+1) -- per-note expression routing
  - Microtonal tuning (+1) -- Scala/TUN, MTS-ESP integration
- **Total:** 2.0 + 9.4 + 5 = 16.4 (capped at **5.0**)

---

## Stages

- Stage 0: Research & Planning -- COMPLETE
- Stage 1: Foundation (project structure, CMakeLists.txt, APVTS parameters)
- Stage 2: DSP -- 5 phases (see below)
- Stage 3: GUI -- 3 phases (see below)
- Stage 4: Validation (presets, pluginval, changelog)

---

## Stage 1: Foundation

**Goal:** Create project structure, CMakeLists.txt, APVTS with all 35 parameters, empty processor/editor shells

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
- `juce_audio_basics` -- audio buffers, MIDI, MPESynthesiser
- `juce_dsp` -- DelayLine, IIR::Filter, Oversampling, ProcessSpec
- `juce_gui_extra` -- WebBrowserComponent (WebView UI)
- `juce_gui_basics` -- basic UI components

**Shared Modules:**
- `modules/tuning/scala-tuning-engine` -- microtonal tuning (link via CMake)

---

## Stage 2: DSP Phases

### Phase 3.1: Static Reed + Cylindrical Bore (Core Engine)

**Goal:** Single monophonic voice producing basic clarinet-like tone with static reed table

**Components:**
- ReedWindVoice (extends juce::MPESynthesiserVoice)
- Static reed table (STK-style piecewise linear with clamp)
- Cylindrical bore waveguide (dual fractional delay lines with Thiran allpass)
- Simple bell reflection filter (one-pole)
- Basic viscothermal loss filter (one-pole)
- Breath pressure from velocity/CC2
- Note-on -> frequency -> delay length
- Mono output path

**Parameters active:** BREATH_PRESSURE, EMBOUCHURE, REED_HARDNESS, REED_OPENING, BELL_SIZE, BORE_DIAMETER, OUTPUT_GAIN

**Test Criteria:**
- [ ] Plugin loads in DAW as instrument (not effect)
- [ ] MIDI note-on produces audible clarinet-like tone
- [ ] BREATH_PRESSURE controls dynamics (soft to loud)
- [ ] EMBOUCHURE affects brightness
- [ ] REED_HARDNESS changes attack character
- [ ] Pitch is accurate across MIDI range (within 1 Hz)
- [ ] Note-off produces natural bore ring-down
- [ ] Sustained tone is stable (no runaway or silence)
- [ ] No clicks or pops during parameter changes
- [ ] Responds to CC2 (breath controller) for pressure

### Phase 3.2: Dynamic Reed + Psi + Conical Bore

**Goal:** Full mass-spring-damper reed, Guillemain Psi for double-reed, conical bore correction

**Components:**
- Full reed ODE (mass, damping, stiffness)
- Guillemain confinement parameter (Psi) in Bernoulli flow
- Polynomial approximation for nonlinear junction
- Conical bore correction filter (Strategy B)
- Breath noise generator
- Mouthpiece volume (Helmholtz compliance)

**Parameters active:** +REED_MASS, REED_DAMPING, DOUBLE_REED, BORE_CHARACTER, AIR_NOISE, MOUTHPIECE_VOL

**Test Criteria:**
- [ ] REED_MASS near zero: sounds similar to Phase 3.1 static reed (convergence)
- [ ] REED_MASS high: attack transients have sluggish onset and reed resonance coloring
- [ ] DOUBLE_REED (Psi) at 0: pure single-reed (same as Phase 3.1)
- [ ] DOUBLE_REED at 0.4: audibly different -- nasal, oboe-like character
- [ ] DOUBLE_REED at 0.7+: piercing, zurna/shehnai character
- [ ] BORE_CHARACTER at 0: odd harmonics (clarinet), overblows at 12th
- [ ] BORE_CHARACTER at 0.8: all harmonics (saxophone), overblows at octave
- [ ] Smooth morph between cylindrical and conical without artifacts
- [ ] AIR_NOISE adds breathiness without dominating tone
- [ ] Reed model stable across full parameter range (no blowups)

### Phase 3.3: Tone Holes + Expression + Legato

**Goal:** Tone hole lattice, register hole, vibrato/growl/flutter, mono legato

**Components:**
- Tone hole lattice (4 virtual holes + register hole)
- Three-port Keefe scattering junctions
- Vibrato system (3 sources: lip, breath, throat)
- Growl/vocal coupling oscillator
- Flutter tongue LFO
- Subtone mode (parameter modifier)
- Attack chiff envelope
- Mono legato mode (smooth delay line interpolation between notes)
- Polyphonic mode (independent voice instances)

**Parameters active:** +TONE_HOLE_CUTOFF, REGISTER_HOLE, VIBRATO_DEPTH, VIBRATO_RATE, VIBRATO_SOURCE, GROWL_AMOUNT, FLUTTER_TONGUE, SUBTONE, ATTACK_CHIFF, BORE_LENGTH, POLY_MODE, MAX_VOICES

**Test Criteria:**
- [ ] TONE_HOLE_CUTOFF shapes spectral envelope (lower = darker)
- [ ] REGISTER_HOLE opens: cylindrical bore jumps to 12th, conical jumps to octave
- [ ] Vibrato modulates correctly for all 3 sources
- [ ] Growl at low amount: subtle beating; at high: multiphonic texture
- [ ] Flutter tongue: ~25 Hz pressure modulation audible
- [ ] Subtone: soft, airy tone without reed beating
- [ ] Attack chiff: note onset has bright burst proportional to velocity
- [ ] Legato: smooth pitch transition between overlapping notes (no re-attack)
- [ ] Polyphonic mode: 8 voices play simultaneously without artifacts
- [ ] CPU usage with 8 voices stays under 25%

### Phase 3.4: Impossible Physics + Dual Bore

**Goal:** Sound design parameters, dual bore drone mode, reverse bore

**Components:**
- Infinite sustain (bore loss reduction)
- Reverse bore (negative taper -- hichiriki extended)
- Dual bore system (second parallel waveguide)
- Drone pitch offset for second bore
- Feedback path (cross-modulation between bores)
- Bore profile choice (simple vs multi-segment)

**Parameters active:** +INFINITE_SUSTAIN, REVERSE_BORE, DUAL_BORE, DRONE_PITCH, FEEDBACK_PATH, BORE_PROFILE

**Test Criteria:**
- [ ] INFINITE_SUSTAIN at 100%: tone sustains indefinitely
- [ ] REVERSE_BORE: unusual timbral character (narrows at bottom)
- [ ] DUAL_BORE enabled: second bore audible at DRONE_PITCH offset
- [ ] DRONE_PITCH at 0: unison with primary bore (chorusing from slight detuning)
- [ ] DRONE_PITCH at -12: drone a fifth below (arghul-like)
- [ ] FEEDBACK_PATH at low values: subtle cross-modulation
- [ ] FEEDBACK_PATH at high values: complex interaction without instability
- [ ] BORE_PROFILE multi-segment: audible difference from simple (throat/body/bell)
- [ ] No runaway or instability with extreme "impossible" settings
- [ ] CPU manageable with dual bore active (< 40% single core mono)

### Phase 3.5: Oversampling + Tuning + MPE + Optimization

**Goal:** 2x/4x oversampling, microtonal tuning integration, full MPE, performance pass

**Components:**
- juce::dsp::Oversampling<float> wrapping reed/junction/bore section
- Tuning engine integration (Scala/TUN, MTS-ESP, 12TET)
- Reference pitch parameter
- OVERSAMPLING choice (2x default / 4x mono)
- MPE routing (pressure -> breath, slide -> embouchure, pitch bend per-note)
- Newton-Raphson solver for 4x mono mode
- Instrument morph system (macro preset crossfade)
- CPU profiling and optimization pass

**Parameters active:** All 35 parameters now active (+REFERENCE_PITCH, TUNING_SYSTEM, OVERSAMPLING, INSTRUMENT_PRESET)

**Test Criteria:**
- [ ] 2x oversampling reduces aliasing (A/B comparison)
- [ ] 4x oversampling mono mode: higher quality, acceptable CPU
- [ ] Latency correctly reported to host
- [ ] Scala file loading works (test pythagorean, just intonation, maqam scales)
- [ ] 12TET default produces standard tuning
- [ ] REFERENCE_PITCH shifts all pitches correctly
- [ ] MPE pressure controls breath per-note
- [ ] MPE slide controls embouchure per-note
- [ ] MPE pitch bend produces smooth portamento
- [ ] Instrument preset morph: Clarinet -> Saxophone -> Duduk smooth transition
- [ ] CPU targets met: mono 2x < 2%, mono 4x < 4%, 8-voice poly < 16%
- [ ] No denormals (ScopedNoDenormals in processBlock)

---

## Stage 3: GUI Phases

### Phase 4.1: Layout and Basic Controls

**Goal:** WebView UI with parameter knobs and layout

**Components:**
- WebView setup (index.html + CSS + JS)
- Parameter binding via WebSliderRelay / WebSliderParameterAttachment
- Tier 1 controls: Breath, Embouchure, Reed Hardness, Bore Character, Instrument Preset
- Tier 2 controls: Reed Opening, Bell Size, Air Noise, Double Reed, Bore Diameter
- Output gain control
- Section grouping (Primary, Secondary, Advanced, Expression, Sound Design, Tuning)

**Test Criteria:**
- [ ] WebView opens with correct size
- [ ] All Tier 1 + Tier 2 knobs visible and styled
- [ ] Knobs respond to mouse drag
- [ ] Layout matches section grouping
- [ ] Background and styling render properly

### Phase 4.2: Parameter Binding and Interaction

**Goal:** Full two-way parameter communication

**Components:**
- JavaScript -> C++ relay calls (all 35 parameters)
- C++ -> JavaScript parameter updates (host automation)
- Choice parameters: INSTRUMENT_PRESET, BORE_PROFILE, VIBRATO_SOURCE, TUNING_SYSTEM, POLY_MODE, OVERSAMPLING
- Bool parameter: DUAL_BORE
- Value formatting and display
- Preset load/save updates all UI elements

**Test Criteria:**
- [ ] All 35 parameters controllable from UI
- [ ] Host automation updates UI controls in real-time
- [ ] Preset changes update all UI elements
- [ ] CHOICE parameters show dropdown/selector
- [ ] BOOL parameter (DUAL_BORE) shows toggle
- [ ] No lag or visual glitches during parameter changes

### Phase 4.3: Advanced UI Elements

**Goal:** Visualization and advanced UI features

**Components:**
- Instrument morph visualization (2D pad or preset selector with visual feedback)
- Bore profile visualization (showing current taper shape)
- Breath controller setup wizard
- Tuning system file browser (Scala/TUN loading)
- Preset browser with instrument categories (Western, Non-Western, Sound Design)
- Tier 3 parameter panel (expandable advanced controls)
- Expression control panel (vibrato, growl, flutter, subtone, chiff)

**Test Criteria:**
- [ ] Instrument morph selector shows current preset with visual feedback
- [ ] Bore visualization responds to BORE_CHARACTER parameter
- [ ] File browser loads Scala/TUN files
- [ ] Preset browser navigates and loads presets by category
- [ ] Advanced panel expands/collapses correctly
- [ ] Expression panel groups all expression controls
- [ ] Performance acceptable (no CPU spikes from UI updates)

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- Reed ODE state (x, x_dot) and bore delay line state are per-voice -- no cross-thread access
- Tuning table: atomic pointer swap from message thread
- Instrument preset target: atomic int, parameter interpolation on audio thread
- Voice allocation via juce::MPESynthesiser handles locking internally

### Performance
- Reed + junction + bore per voice (2x OS): ~178 ops/output sample (~0.2% CPU)
- Tone holes (4): ~40 ops/output sample
- Expression modulation: ~15 ops/output sample
- Total per voice (2x OS): ~233 ops/output sample
- 8-voice polyphonic: ~1864 ops/output sample (~2% CPU)
- Mono 4x OS: ~466 ops/output sample (~0.5% CPU)
- Dual bore: approximately doubles bore section (~120 additional ops)
- Budget: comfortable for 8-16 voice polyphony at 2x OS

### Latency
- Waveguide: 0 algorithmic latency (causal)
- 2x Oversampling: ~8 samples (halfband filter group delay)
- 4x Oversampling: ~16 samples
- Report via `setLatencySamples()` in `prepareToPlay()`
- NOTE: getLatencySamples() is NOT virtual in JUCE 8

### Denormal Protection
- Use `juce::ScopedNoDenormals` in processBlock()
- Clamp reed displacement and velocity to prevent tiny values
- Add epsilon to sqrt: `sqrt(max(|dp|, 1e-10))`
- juce::dsp::DelayLine and juce::dsp::IIR::Filter handle denormals internally

### Known Challenges
- Newton-Raphson divergence: clamp iterations to 4, bail out with previous solution
- Conical bore correction filter: may need iterative tuning per instrument preset for accuracy
- Psi stability at extreme values: clamp effective range if instability detected
- Bore morphing during playback: smooth coefficient interpolation over ~50ms to prevent clicks
- Reference implementations: STK BlowHole.cpp, STK Saxofony.cpp, Faust pm.lib reed models
- O-Bowed provides proven MPESynthesiserVoice + TuningEngine integration pattern
- O-Lyrica WaveguideString pattern directly applicable to bore waveguide

---

## References

- Creative brief: `plugins/O-Reed/.planning/BRIEF.md`
- Parameter spec: `plugins/O-Reed/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-Reed/.planning/research/ARCHITECTURE.md`

### Research Documents
- `research/reed-physical-modeling-dsp.md` -- Full DSP algorithms with C++ pseudocode
- `research/O-Reed-market-research.md` -- SWAM/Respiro/Chromaphone analysis, pricing
- `research/O-Reed-acoustic-properties-reed-instruments.md` -- Bore dimensions, extended techniques
- `research/O-Reed-research-synthesis.md` -- Unified findings, 4-stage DSP roadmap
- `research/physical-modeling-research-agent-3-physical-modelling-optimization.md` -- CPU optimization guide

### Reference Plugins in Codebase
- O-Bowed -- Same architecture pattern: MPESynthesiserVoice, waveguide, TuningEngine, tiered nonlinearity
- O-Lyrica -- WaveguideString, TuningEngine integration, HarpSynthVoice pattern
- O-Formant -- MPESynthesiserVoice with 6 pure virtuals, custom biquad structs for many filter instances
- O-Prism -- TuningEngine shared module usage, complex synth ROADMAP pattern
