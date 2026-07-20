# DrumRoulette - Implementation Plan

**Date:** 2025-11-12
**Complexity Score:** 5.0 (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

Complexity Calculation:
- **Parameters:** 73 parameters (73/5 = 14.6 points, capped at 2.0) = **2.0**
- **Algorithms:** 8 DSP components = **8**
  - Voice Architecture (juce::Synthesiser + custom voices)
  - Sample Playback Engine (AudioFormatManager + AudioFormatReader)
  - Envelope Generator (juce::ADSR per voice)
  - Tilt Filter (IIR::Filter cascaded shelving)
  - Folder Randomization (Custom File scanning)
  - Multi-Output Routing (Bus configuration)
  - Solo/Mute Logic (Custom processBlock logic)
  - Volume Control (dsp::Gain)
- **Features:** 2 points
  - Modulation systems: ADSR envelopes per voice (+1)
  - External MIDI control: 8 note triggers C1-G1 (+1)
- **Total:** 12.0 (capped at **5.0**)

**Result:** Maximum complexity (5.0/5.0) - Requires phased implementation with careful testing at each stage.

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ✓
- Stage 2: Foundation ← Next
- Stage 3: Shell (73 parameters)
- Stage 4: DSP (4 phases)
- Stage 5: GUI (3 phases)
- Stage 6: Validation

---

## Stage 4: DSP Phases

### Phase 4.1: Voice Architecture and Sample Playback

**Goal:** Implement core synthesis framework with basic sample playback

**Components:**
- juce::Synthesiser setup with 8 custom SynthesiserVoice subclasses
- MIDI note mapping (C1-G1 → Slots 1-8)
- AudioFormatManager registration (WAV, AIFF, MP3, AAC)
- Sample loading into AudioSampleBuffer
- Basic sample playback with velocity scaling
- Multi-output bus configuration (18 channels: 2 main + 16 individual)

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] All 8 MIDI notes trigger correct slots
- [ ] Sample playback audible on both main and individual outputs
- [ ] Velocity sensitivity works (soft/loud notes have different volumes)
- [ ] Multi-output routing correct (each slot on expected bus)

**Duration:** 45 min

---

### Phase 4.2: Envelope and Pitch Shifting

**Goal:** Add ADSR envelopes and pitch shifting to each voice

**Components:**
- juce::ADSR integration per voice (sustain=0, release=50ms fixed)
- ATTACK_N and DECAY_N parameter connections
- Pitch shifting via variable-rate playback (pow(2.0, semitones / 12.0))
- Linear interpolation for fractional sample positions
- PITCH_N parameter connections

**Test Criteria:**
- [ ] ATTACK parameter shapes note onset smoothly
- [ ] DECAY parameter controls tail length correctly
- [ ] Envelope reaches zero when decay completes
- [ ] PITCH parameter shifts pitch by correct semitones (±12)
- [ ] No clicking or artifacts at extreme pitch values

**Duration:** 30 min

---

### Phase 4.3: Filtering and Volume Control

**Goal:** Add tilt filter and per-channel volume control

**Components:**
- Tilt filter implementation (cascaded low-shelf + high-shelf at 1kHz)
- TILT_FILTER_N parameter connections (±12dB)
- Volume control using juce::dsp::Gain
- VOLUME_N parameter connections (-60dB to +6dB)
- Gain conversion (Decibels::decibelsToGain, -60dB = silence)

**Test Criteria:**
- [ ] TILT_FILTER at 0dB produces flat response
- [ ] Positive tilt values brighten sound (boost highs, cut lows)
- [ ] Negative tilt values darken sound (boost lows, cut highs)
- [ ] VOLUME fader controls channel level smoothly
- [ ] -60dB position produces silence (no denormals)

**Duration:** 30 min

---

### Phase 4.4: Folder Randomization and Solo/Mute Logic

**Goal:** Implement folder scanning, randomization, and solo/mute routing

**Components:**
- Recursive folder scanning (File::findChildFiles with recurse=true)
- Random sample selection (Random::getSystemRandom().nextInt())
- FOLDER_PATH_N, RANDOMIZE_N, LOCK_N button handling
- RANDOMIZE_ALL global button (skip if LOCK_N = true)
- Solo/Mute logic for main mix (individual outputs unaffected)
- SOLO_N and MUTE_N parameter connections
- Thread safety: File I/O on message thread, not audio thread

**Test Criteria:**
- [ ] FOLDER button opens file browser
- [ ] RANDOMIZE button loads random sample from folder
- [ ] RANDOMIZE_ALL button randomizes all unlocked slots
- [ ] LOCK button prevents global randomization
- [ ] SOLO button isolates single channel on main mix
- [ ] MUTE button silences channel on main mix
- [ ] Individual outputs always active (ignore SOLO/MUTE)
- [ ] No crashes or file errors with empty folders

**Duration:** 45 min

---

## Stage 5: GUI Phases

### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate mockup HTML and establish WebView infrastructure

**Components:**
- Copy v4-ui.html to Source/ui/public/index.html
- Update PluginEditor.h/cpp with WebView setup from v4 templates
- Configure CMakeLists.txt for WebView resources (choc_WebView integration)
- Implement basic relay system for parameter communication
- Layout verification: 8 vertical channel strips, 1400×950px window

**Test Criteria:**
- [ ] WebView window opens with correct size (1400×950px)
- [ ] All 8 channel strips visible with correct spacing
- [ ] Master section renders correctly
- [ ] Brushed metal texture and styling match mockup
- [ ] No console errors or resource loading failures

**Duration:** 30 min

---

### Phase 5.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI ↔ DSP)

**Components:**
- JavaScript → C++ relay calls for all 73 parameters
- C++ → JavaScript parameter updates (host automation support)
- Value formatting with units (dB, ms, semitones)
- Fader binding (VOLUME_N × 8)
- Knob binding (DECAY_N, ATTACK_N, TILT_FILTER_N, PITCH_N × 8 each)
- Button binding (FOLDER_PATH_N, RANDOMIZE_N, LOCK_N, RANDOMIZE_ALL)
- Toggle binding (SOLO_N, MUTE_N × 8 each)
- Sample name display updates

**Test Criteria:**
- [ ] All 73 parameters respond to UI control movements
- [ ] Host automation updates UI controls in real-time
- [ ] Preset changes update all UI elements correctly
- [ ] Parameter values display with correct units
- [ ] No lag or visual glitches during rapid parameter changes
- [ ] LOCK button changes color when toggled (grey→orange)
- [ ] SOLO/MUTE buttons visually indicate state

**Duration:** 60 min

---

### Phase 5.3: LED Indicators and Polish

**Goal:** Add trigger indicators and final visual polish

**Components:**
- LED-style trigger indicators per channel (velocity-sensitive brightness)
- MIDI note-on → LED pulse animation
- Sample name display per channel (truncate long filenames)
- Master section polish (global randomize button styling)
- Rack mounting holes and hardware aesthetic details
- Error message display for empty folders

**Test Criteria:**
- [ ] LED indicators light up on MIDI note-on
- [ ] LED brightness scales with MIDI velocity
- [ ] Sample names display correctly (truncated if long)
- [ ] Global randomize button prominently styled
- [ ] All visual details match mockup v4
- [ ] Error messages appear for invalid folder paths

**Duration:** 30 min

---

## Estimated Duration

Total: ~4 hours 30 minutes

- Stage 2: 5 min (Foundation - project structure)
- Stage 3: 15 min (Shell - 73 APVTS parameters)
- Stage 4: 150 min (DSP - 4 phases)
  - Phase 4.1: 45 min (Voice architecture + sample playback)
  - Phase 4.2: 30 min (Envelope + pitch shifting)
  - Phase 4.3: 30 min (Filtering + volume)
  - Phase 4.4: 45 min (Randomization + solo/mute)
- Stage 5: 120 min (GUI - 3 phases)
  - Phase 5.1: 30 min (Layout + basic controls)
  - Phase 5.2: 60 min (Parameter binding)
  - Phase 5.3: 30 min (LED indicators + polish)
- Stage 6: 20 min (Validation - presets, pluginval, changelog)

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- Sample loading: Use `MessageManager::callAsync()` to defer file I/O to message thread
- Voice rendering: Lock-free (no allocations, no file access in `processBlock()`)
- Folder randomization buttons: Trigger async file scan, update sample buffer when ready
- MIDI note-on: Safe to trigger from audio thread (synthesizer handles allocation)

### Performance
- Sample playback: ~5% CPU per active voice (linear interpolation, ADSR, filter)
- Total estimated: ~40% single core with 8 voices playing simultaneously
- Optimization: Pre-load samples into AudioSampleBuffer (no disk I/O during playback)
- Optimization: Use FloatVectorOperations for buffer operations where applicable
- Multi-output: Minimal overhead (bus routing is pointer-based)

### Latency
- Zero additional latency (direct sample playback, no look-ahead)
- Filter delay: ~1 sample (first-order IIR, negligible)
- Report `getLatencySamples()` = 0 (no host compensation needed)

### Denormal Protection
- Use `juce::ScopedNoDenormals` in `processBlock()`
- ADSR envelope: JUCE handles denormals internally
- Filter state: Add DC blocker if denormals detected in testing
- Volume control: Gain below -60dB treated as silence (no multiplication)

### Known Challenges

**Multi-Output Routing:**
- Some DAWs may not support 9 output buses (fallback to main only)
- Bus layout must be validated in `isBusesLayoutSupported()`
- Buffer routing uses `getBusBuffer(busIndex)` for specific outputs
- Reference: Battery 4 (8 mono + 8 stereo), Kontakt (flexible routing)

**Folder Randomization:**
- File I/O must happen on message thread (use `MessageManager::callAsync()`)
- Handle edge case: Empty folder or no valid audio files (display error)
- Handle edge case: Recursive scan with deeply nested folders (limit depth?)
- Sample loading: Convert to plugin's native sample rate if needed
- Reference: Geist2 (preview output routing pattern)

**Pitch Shifting:**
- Linear interpolation: Fast, acceptable quality for ±12 semitones
- Edge case: Playback rate near 0 or very high (boundary checks needed)
- Edge case: Sample shorter than attack time (clamp envelope)
- Reference: Kontakt (variable-rate resampling, similar approach)

**ADSR with Sustain=0:**
- Decay must complete before voice can be freed
- Use `ADSR::isActive()` to check when envelope silent
- Release phase (50ms) prevents clicks on note-off
- Reference: Percussive envelope pattern (common in drum samplers)

**Solo/Mute Logic:**
- Priority: SOLO overrides MUTE (if slot soloed, MUTE ignored)
- Main mix affected, individual outputs always active
- Global check: If ANY slot has SOLO=true, only soloed slots audible
- Reference: Mixing console behavior (standard SSL/Neve pattern)

**WebView Parameter Binding:**
- 73 parameters: Requires efficient relay system to avoid boilerplate
- Use pattern from FlutterVerb/LushVerb: Relay array with indexed access
- Button parameters (FOLDER_PATH, RANDOMIZE): Custom handling, not APVTS
- Sample name display: Update via separate relay (not a parameter)

---

## References

**Contract Files:**
- Creative brief: `plugins/DrumRoulette/.ideas/creative-brief.md`
- Parameter spec: `plugins/DrumRoulette/.ideas/parameter-spec.md`
- DSP architecture: `plugins/DrumRoulette/.ideas/architecture.md`
- UI mockup: `plugins/DrumRoulette/.ideas/mockups/v4-ui.yaml`

**Similar Plugins (Reference):**
- **OrganicHats** - Synthesiser-based instrument, MIDI note mapping, ADSR envelopes
- **FlutterVerb** - WebView UI with 7 parameters, relay system, VU meter
- **LushVerb** - Multi-parameter binding (4 knobs), parameter value displays
- **GainKnob** - Filter state reset pattern (prevents bursts on type change)
- **TapeAge** - ADSR envelope with sustain=0 pattern (decay-focused)

**JUCE Documentation:**
- Synthesiser class: Voice allocation and polyphony management
- AudioFormatManager: Multi-format audio file loading
- ADSR: Envelope generator with attack, decay, sustain, release
- IIR::Filter: Biquad filter with shelving coefficient helpers
- dsp::Gain: Simple gain processor with decibel conversion

**Professional Plugins (Research):**
- Native Instruments Battery 4: Multi-slot drum sampler architecture
- FXpansion Geist2: Multi-output routing (up to 64 channels)
- Native Instruments Kontakt: Pitch shifting via resampling
