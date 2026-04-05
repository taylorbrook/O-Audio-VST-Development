# O-Wind - Implementation Plan

**Date:** 2026-04-04
**Complexity Score:** 5.0 (Maximum -- raw score 11.6 capped at 5.0)
**Strategy:** Phase-based implementation
**Complexity Tier:** 4 (Physical modeling synth with custom DSP, MPE, microtonal, tiered architecture)

---

## Complexity Factors

- **Parameters:** 13 parameters (13/5 = 2.6, capped at 2.0) = **2.0**
- **Algorithms:** 7 DSP components = **7**
  1. Jet Exciter (custom breath/embouchure model)
  2. Jet Delay Line (juce::dsp::DelayLine<Lagrange3rd>)
  3. Jet-Labium Nonlinearity (custom tanh saturation)
  4. Bore Waveguide (juce::dsp::DelayLine<Thiran> x2)
  5. Bore Loss + End Reflection + Radiation Filters (juce::dsp::IIR::Filter x3)
  6. Tone Hole System (two-tier: bore length switching + optional Keefe scattering)
  7. 2x Oversampling (juce::dsp::Oversampling)
- **Features:** 2.6 points
  - Feedback loop (+1) -- bore-to-jet feedback path
  - Modulation system (+1) -- vibrato LFO on breath pressure
  - MPE support (+0.6) -- per-note expression (shared module, reduced weight)
- **Total:** 2.0 + 7 + 2.6 = 11.6 (capped at **5.0**)

---

## Stages

- Stage 0: Research & Planning -- COMPLETE
- Stage 1: Foundation (project structure, CMakeLists.txt, APVTS parameters)
- Stage 2: DSP -- 4 phases (see below)
- Stage 3: GUI -- 3 phases (see below)
- Stage 4: Validation (presets, pluginval, changelog)

---

## Stage 1: Foundation

**Goal:** Create project structure, CMakeLists.txt, APVTS with all 13 parameters, empty processor/editor shells

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
- `juce_audio_basics` -- audio buffers, MIDI, SmoothedValue, Synthesiser
- `juce_dsp` -- DelayLine, IIR::Filter, Oversampling, ProcessSpec
- `juce_gui_extra` -- WebBrowserComponent (WebView UI)
- `juce_gui_basics` -- basic UI components

**Shared Modules:**
- `modules/tuning/scala-tuning-engine` -- microtonal tuning (link via CMake)

---

## Stage 2: DSP Phases

### Phase 3.1: Minimal Oscillating Model

**Goal:** Single voice producing sound with jet + bore + feedback. Validate self-oscillation and pitch tracking.

**Components:**
- FluteSynthVoice (extends juce::SynthesiserVoice)
- Jet exciter: breath pressure -> jet velocity (simplified, no noise yet)
- Jet delay line (Lagrange3rd)
- Jet nonlinearity (tanh)
- DC blocker
- Bore waveguide (Thiran, bidirectional)
- Bore loss filter (one-pole lowpass, fixed coefficients)
- End reflection filter (one-pole lowpass with sign inversion)
- Radiation filter (one-pole highpass)
- Feedback path: bore output -> embouchure summation (previous sample)
- Tier 1 tone holes: MIDI note -> bore delay length (no crossfade yet)
- Monophonic (1 voice)
- No oversampling yet (add in Phase 3.2)

**Parameters active:** BREATH_PRESSURE, EMBOUCHURE, TONE_COLOR, JET_REFLECTION, END_REFLECTION, OUTPUT_LEVEL

**Test Criteria:**
- [ ] Plugin loads in DAW as instrument (not effect)
- [ ] MIDI note-on produces audible flute-like tone
- [ ] Pitch tracks correctly across MIDI range (C4-C7 minimum)
- [ ] BREATH_PRESSURE affects volume and tone quality
- [ ] EMBOUCHURE changes register (fundamental vs overblow at extremes)
- [ ] TONE_COLOR affects brightness
- [ ] JET_REFLECTION and END_REFLECTION affect sustain character
- [ ] Note-off produces natural decay (breath release)
- [ ] No DC drift or runaway oscillation
- [ ] No clicks on note transitions (basic)

---

### Phase 3.2: Expression + Oversampling

**Goal:** Add breath noise, vibrato, oversampling, parameter smoothing, polyphony, crossfade tone holes

**Components:**
- Turbulence noise generator (juce::Random + lowpass filter, scales with U_j^2)
- Vibrato LFO (pressure modulation, NOT pitch)
- 2x oversampling wrapping entire waveguide loop
- SmoothedValue on bore delay for Tier 1 tone hole crossfade (2-5ms)
- SmoothedValue on embouchure parameter (prevents register jumps)
- Polyphony: 4 voices (default), configurable up to 8
- Breath attack ramp from velocity (5-30ms)
- Breath release envelope on note-off (20-100ms)
- CC2/Aftertouch -> breath pressure mapping
- CC74/MPE Y -> embouchure mapping

**Parameters active:** All 13 except REVERSED_JET, SUB_HARMONICS, INFINITE_SUSTAIN

**Test Criteria:**
- [ ] Breath noise audible and scales with playing dynamics
- [ ] Vibrato sounds natural (pitch + amplitude modulation from pressure vibrato)
- [ ] No aliasing artifacts (oversampling working)
- [ ] Tone hole transitions are click-free (crossfade)
- [ ] 4-voice polyphony works without CPU spikes
- [ ] CC2 (breath controller) maps to dynamics
- [ ] CC74 maps to embouchure/timbre
- [ ] Velocity affects attack character
- [ ] Latency reported correctly to host

---

### Phase 3.3: Impossible Physics + Instrument Presets

**Goal:** Add creative parameters and instrument preset system

**Components:**
- INFINITE_SUSTAIN: bore loss filter gain approaches 1.0
- REVERSED_JET: blend between normal and inverted tanh
- SUB_HARMONICS: nonlinear feedback injection for sub-octave
- Instrument preset system: parameter-set switching
  - Concert Flute (core)
  - Shakuhachi (core)
  - Bansuri (core)
  - Native American Flute (core)
- Internal parameters per preset (jet gain, noise spectrum, radiation cutoff, bore characteristics)
- Stereo width processor (shared pattern with O-Bowed)
- AIR_COLUMN parameter connection

**Parameters active:** All 13

**Test Criteria:**
- [ ] INFINITE_SUSTAIN creates drone-like endless decay
- [ ] REVERSED_JET produces unusual timbres (not crashes)
- [ ] SUB_HARMONICS adds audible sub-octave content
- [ ] Each instrument preset sounds distinctly different
- [ ] Shakuhachi has breathy, expressive character
- [ ] Bansuri has warm, airy quality
- [ ] Native American flute has warm, haunting tone
- [ ] Concert flute is clear and projecting
- [ ] Stereo width works (mono to wide)
- [ ] Preset switching is click-free

---

### Phase 3.4: Advanced Features (Optional Enhancement)

**Goal:** Tier 2 tone holes, expansion presets, MPE refinement, tuning system integration

**Components:**
- Tier 2 tone holes: Keefe 3-port scattering junctions (6-8 per voice)
  - Second-order IIR filters per junction
  - Half-holing support for pitch bending
  - Cross-fingering for alternate timbres
- Expansion instrument presets: Recorder, Pan Flute, Piccolo, Ocarina
- Full MPE integration (per-note pitch bend, slide, pressure)
- Tuning system integration (Scala/TUN, MTS-ESP via shared module)
- Pitch bend -> smooth bore delay modulation (portamento)
- CC1 -> vibrato depth mapping

**Test Criteria:**
- [ ] Tier 2 tone holes produce more realistic timbral variation
- [ ] Half-holing enables smooth pitch bends (shakuhachi meri/kari)
- [ ] Cross-fingering produces alternate timbres for same pitch
- [ ] Expansion presets are playable and distinct
- [ ] MPE per-note expression works correctly
- [ ] Tuning system accepts Scala files
- [ ] CPU within budget (4 voices < 14% with Tier 2)
- [ ] No stability issues with full tone hole scattering

---

## Stage 3: GUI Phases

### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate mockup HTML and bind basic parameters

**Components:**
- Copy UI mockup to Source/ui/public/index.html
- Update PluginEditor.h/cpp with WebView setup
- Configure CMakeLists.txt for WebView resources (NEEDS_WEB_BROWSER TRUE)
- Bind primary parameters via relay system (13 knobs/sliders)
- Instrument preset selector (dropdown or visual picker)

**Test Criteria:**
- [ ] WebView window opens with correct size
- [ ] All 13 parameter controls visible and styled
- [ ] Layout matches mockup design
- [ ] Instrument preset selector visible

---

### Phase 5.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI <-> DSP)

**Components:**
- JavaScript -> C++ relay calls (control changes)
- C++ -> JavaScript parameter updates (host automation)
- Value formatting and display (Hz, dB, %, etc.)
- Instrument preset change from UI
- Real-time parameter updates during playback

**Test Criteria:**
- [ ] Control movements change DSP parameters
- [ ] Host automation updates UI controls
- [ ] Preset changes update all UI elements
- [ ] Parameter values display correctly with units
- [ ] No lag or visual glitches

---

### Phase 5.3: Advanced UI Elements

**Goal:** Visual feedback and polish

**Components:**
- Breath/jet visualization (real-time indicator of excitation state)
- Register indicator (which harmonic mode is active)
- Instrument preset visual (image or icon per instrument)
- Tone hole state visualization (if Tier 2 implemented)
- MIDI activity indicator

**Test Criteria:**
- [ ] Breath visualization responds to playing dynamics
- [ ] Register indicator updates correctly during overblowing
- [ ] Visual polish matches O-series standard
- [ ] Performance acceptable (no CPU spikes from UI updates)

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- Bore loss filter coefficient updates on parameter change (not per-sample)
- Instrument preset coefficients swapped via atomic pointer
- Tuning table swapped atomically
- No allocations or mutexes on audio thread

### Performance
- Jet exciter + bore waveguide: ~2.0-2.5% per voice (Tier 1)
- With Tier 2 tone holes: ~2.5-3.5% per voice
- 4 voices total: ~10-14% (well within budget)
- 8 voices total: ~20-28% (feasible on modern CPUs)
- 30-50% cheaper per voice than O-Bowed

### Latency
- Algorithmic: 0 samples (causal waveguide)
- Oversampling: ~8 samples (2x polyphase halfband)
- Report total via `setLatencySamples()` in prepareToPlay()
- NOTE: `getLatencySamples()` is NOT virtual in JUCE 8

### Denormal Protection
- Use `juce::ScopedNoDenormals` in processBlock()
- DC blocker inside waveguide prevents DC accumulation
- JUCE dsp::IIR::Filter handles denormals internally

### Known Challenges
- Overblowing stability across full MIDI range -- test each instrument preset thoroughly
- Shakuhachi embouchure sensitivity -- may need wider embouchure parameter range
- Tone hole Tier 2 filter stability during rapid transitions -- use coefficient recalculation from freq/Q, not raw lerp
- Native American flute dual-chamber modeling -- may need additional short bore stub or simplified approach
- Pan flute preset -- individual stopped pipes (closed-end) vs open-end bore requires different reflection filter sign

### Comparison with O-Bowed

| Aspect | O-Bowed | O-Wind |
|--------|---------|--------|
| Excitation | Bidirectional friction (NR solver) | One-directional jet (no solver) |
| Body | Separate 8-biquad morphable resonator | Bore IS the body (no resonator) |
| Parameters | 22 | 13 |
| Raw complexity | 18.0 | 11.6 |
| Per-voice CPU | ~2-5% | ~2-2.5% |
| DSP phases | 5 | 4 |
| Highest risk | Friction junction stability | Overblowing register stability |

---

## References

- Creative brief: `plugins/O-Wind/.planning/BRIEF.md`
- Parameter spec: `plugins/O-Wind/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-Wind/.planning/research/ARCHITECTURE.md`
- Requirements: `plugins/O-Wind/.planning/REQUIREMENTS.md`

**Pre-existing research:**
- `research/flute-physical-modeling-synthesis.md` -- jet-drive physics, bore model, tone holes, instrument variants
- `research/O-Wind-market-research.md` -- competitive landscape, market gaps, pricing
- `research/flute-waveguide-juce8-implementation.md` -- JUCE 8 class mapping, CPU estimates, O-Bowed comparison

**Reference plugins:**
- O-Bowed -- shared physical modeling patterns, MPE routing, tuning engine
- O-Lyrica -- SynthesiserVoice pattern with TuningEngine pointer and APVTS pointer
- O-Formant -- MPE with standard Synthesiser (manual CC74/Aftertouch routing)
