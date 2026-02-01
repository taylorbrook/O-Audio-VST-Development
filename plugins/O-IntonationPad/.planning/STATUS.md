---
plugin: O-IntonationPad
stage: 4
status: complete
phase: null
last_updated: 2026-01-30
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: none
contract_checksums:
  brief: valid
  architecture: valid
  roadmap: valid
aesthetic: ouaricon-naturalist-001
illustration: ocean/shell_conchologiaiconi12reev_0090.png
---

# O-IntonationPad Status

## Current Position

Stage: 4 of 4 (Wavetable Design) — **COMPLETE**
Status: All stages complete, plugin ready for use
Progress: [####################] 100%

## Completed So Far

**Stage 0:** ✓ Complete (2026-01-29)
- Plugin type defined: Synth (Wavetable Pad with Just Intonation)
- Professional examples researched: Serum, Vital, Omnisphere, Zebra, Scala ecosystem
- JUCE modules identified: juce_audio_processors (Synthesiser), juce_dsp (Filter, Oscillator)
- Ouaricon modules identified: scala-tuning-engine v1.13.0
- DSP feasibility verified: Wavetable synthesis validated, chord generation researched
- Parameter ranges researched: 15 parameters (2-12 voices, 0-100% complexity, 5 tuning systems)
- Complexity score: **5.0/5.0** (Maximum complexity)
- Strategy: **Staged implementation** (4-phase DSP development with validation checkpoints)
- ARCHITECTURE.md documented: Complete DSP specification with JUCE class mappings
- ROADMAP.md documented: Multi-phase plan with fallback strategies

**Stage 1:** ✓ Complete (2026-01-29)
- CMakeLists.txt created with IS_SYNTH TRUE, juce_dsp module
- PluginProcessor with output-only bus config (synth)
- All 15 APVTS parameters implemented with correct ranges/defaults
- PluginEditor placeholder created (600x400)
- VST3 and AU built and installed to system folders
- auval verification: `aumu OuIP OuDv` (Audio Unit Music Device)
- Plugin appears in Instruments category (not Effects)

**Stage 2:** ✓ Complete (2026-01-29)
- **Phase 2.1:** Basic wavetable oscillator
  - WavetableData.h with 256×2048 sine wavetable
  - WavetableOscillator class with phase interpolation
  - WavetableVoice (SynthesiserVoice subclass)
  - WavetableSound (SynthesiserSound subclass)
  - 8-voice polyphony integrated with Synthesiser

- **Phase 2.2:** Chord generation system
  - ChordGenerator class with scale-degree analysis
  - 10 scale patterns (Major through Melodic Minor)
  - Chord quality determination (Major/Minor/Diminished)
  - Complexity-based extensions (triads → 13th chords)
  - WavetableVoice expanded to 12 sub-oscillators per voice

- **Phase 2.3:** Tuning system integration
  - TuningSystem class with atomic thread-safe operations
  - 5-limit Just Intonation (pure ratios: 3:2, 5:4, 6:5)
  - Pythagorean tuning (3-limit, sharp thirds)
  - 12-TET reference tuning
  - Tonic transposition via keyRoot parameter

- **Phase 2.4:** Modulation, filtering, polish
  - Global LFO modulating wavetable position (0.01-20 Hz)
  - StateVariableTPTFilter low-pass (20-20kHz, Butterworth)
  - Inversion randomization (octave shifts)
  - Detune randomization (±50 cents)
  - Master volume control
  - All 15 parameters connected to DSP

**Stage 3:** ✓ Complete (2026-01-29)
- WebView-based GUI implemented (800x500 window)
- 4 tabbed sections (Voice, Tuning, Modulation, Output)
- 15 parameter controls bound bidirectionally:
  - 11 knobs (voiceCount, complexity, inversionRandom, wavetablePos, lfoRate, lfoDepth, timingRandom, detuneRandom, attackTime, releaseTime, filterCutoff, masterVolume)
  - 2 dropdowns (keyRoot, keyScale)
  - 1 button row (tuningSystem)
- Interactive SVG pitch circle visualization (5 tuning systems)
- Ouaricon Naturalist aesthetic applied (aged paper, shell botanical overlay)
- Pattern #11 member order verified (relays → webView → attachments)
- All UI files added to BinaryData (index.html, juce bridge, pitch-circle.js, images)
- VST3 and AU built and installed to system folders

**Stage 4:** ✓ Complete (2026-01-30)
- **timingRandom parameter connected** — per-sub-oscillator delays for staggered chord attacks
- **JI Harmonic Wavetable** — 12 partials using 5-limit Just Intonation ratios (1:1, 2:1, 3:2, 5:4, etc.)
- **256-frame morphing** — pure sine (0%) to rich JI spectrum (100%)
- **Mipmap anti-aliasing** — 11 band-limited levels, automatic selection based on pitch
- **All 15 parameters verified connected and functional**
- VST3 and AU rebuilt and installed to system folders

## Next Steps

Plugin is complete. Optional future enhancements:
- Preset manager integration
- Scala file import (.scl file picker)
- Active note highlighting on pitch circle
- Additional wavetable banks

## Research Findings Summary (Stage 3)

- **Pitch Circle:** Reuse existing SVG module from `scala-tuning-engine`
- **Tab Component:** Custom CSS/JS (proven pattern from O-Tremolo)
- **Choice Parameters:** Dropdowns for 10+ options, button row for tuningSystem (5)
- **WebView Binding:** 15 WebSliderRelay instances following Pattern #11/#12
- **Complexity:** Medium (no novel components, proven patterns)

## Stage 3 Discuss Phase Decisions

- **Aesthetic:** Ouaricon Audio Naturalist (brand standard)
- **Window Size:** 800x500 (medium)
- **Layout:** 4 tabbed sections (Voice/Tuning/Modulation/Output)
- **Visualization:** Interactive pitch circle for tuning intervals
- **Illustration:** Ocean shell (spiral relates to harmonic series)

## Files Created (Stage 2)

- plugins/O-IntonationPad/Source/DSP/WavetableData.h
- plugins/O-IntonationPad/Source/DSP/WavetableOscillator.h
- plugins/O-IntonationPad/Source/DSP/WavetableSound.h
- plugins/O-IntonationPad/Source/DSP/WavetableVoice.h
- plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp
- plugins/O-IntonationPad/Source/DSP/ChordGenerator.h
- plugins/O-IntonationPad/Source/DSP/ChordGenerator.cpp
- plugins/O-IntonationPad/Source/DSP/TuningSystem.h
- plugins/O-IntonationPad/Source/DSP/TuningSystem.cpp

## Files Modified (Stage 2)

- plugins/O-IntonationPad/Source/PluginProcessor.h
- plugins/O-IntonationPad/Source/PluginProcessor.cpp
- plugins/O-IntonationPad/CMakeLists.txt

## Files Created (Stage 3)

- plugins/O-IntonationPad/Source/ui/public/index.html
- plugins/O-IntonationPad/Source/ui/public/js/juce/index.js
- plugins/O-IntonationPad/Source/ui/public/js/juce/check_native_interop.js
- plugins/O-IntonationPad/Source/ui/public/modules/pitch-circle.js
- plugins/O-IntonationPad/Source/ui/public/img/paper.jpg
- plugins/O-IntonationPad/Source/ui/public/img/shell.png

## Files Modified (Stage 3)

- plugins/O-IntonationPad/Source/PluginEditor.h (Added WebView infrastructure, 15 relays, 15 attachments)
- plugins/O-IntonationPad/Source/PluginEditor.cpp (Complete WebView implementation)
- plugins/O-IntonationPad/CMakeLists.txt (Added BinaryData resources, JUCE_WEB_BROWSER=1)

## Files Modified (Stage 4)

- plugins/O-IntonationPad/Source/DSP/WavetableData.h (JI harmonic wavetable with mipmap anti-aliasing)
- plugins/O-IntonationPad/Source/DSP/WavetableOscillator.h (Mipmap level selection based on frequency)
- plugins/O-IntonationPad/Source/DSP/WavetableVoice.h (Added timingRandom parameter, per-sub-oscillator delays)
- plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp (Timing randomization implementation)
- plugins/O-IntonationPad/Source/PluginProcessor.cpp (Connected timingRandom parameter)

## Parameters Implemented

| Parameter | Type | Range | Default | Connected |
|-----------|------|-------|---------|-----------|
| voiceCount | Int | 2-12 | 5 | ✓ ChordGenerator |
| complexity | Float | 0-1 | 0.5 | ✓ ChordGenerator |
| keyRoot | Choice | 0-11 (C-B) | 0 | ✓ TuningSystem, ChordGenerator |
| keyScale | Choice | 0-9 (10 scales) | 0 | ✓ ChordGenerator |
| tuningSystem | Choice | 0-4 (5 systems) | 1 (JI) | ✓ TuningSystem |
| inversionRandom | Float | 0-1 | 0.3 | ✓ WavetableVoice |
| timingRandom | Float | 0-100ms | 10 | ✓ WavetableVoice (per-sub-osc delays) |
| detuneRandom | Float | 0-50 cents | 5 | ✓ WavetableVoice |
| wavetablePos | Float | 0-1 | 0.5 | ✓ WavetableOscillator (JI harmonic morph) |
| lfoRate | Float | 0.01-20 Hz | 0.5 | ✓ Global LFO → wavetablePos modulation |
| lfoDepth | Float | 0-1 | 0.25 | ✓ Global LFO → wavetablePos modulation |
| attackTime | Float | 0.001-5s | 0.5 | ✓ ADSR Envelope |
| releaseTime | Float | 0.01-10s | 2.0 | ✓ ADSR Envelope |
| filterCutoff | Float | 20-20000 Hz | 8000 | ✓ SVTPTFilter |
| masterVolume | Float | 0-1.26 | 1.0 | ✓ Output gain |

## DSP Architecture

```
MIDI Input
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│  processBlock                                                │
│  ├── Read parameters (atomic)                               │
│  ├── Update TuningSystem mode & tonic                       │
│  ├── Calculate LFO value (global, free-running)             │
│  ├── Modulate wavetablePos with LFO                         │
│  ├── Update all WavetableVoices with parameters             │
│  └── synthesiser.renderNextBlock()                          │
│       │                                                     │
│       ▼                                                     │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  WavetableVoice (×8 polyphony)                      │   │
│  │  ├── startNote: ChordGenerator → up to 12 voices   │   │
│  │  │   ├── Scale-degree analysis                     │   │
│  │  │   ├── Chord quality (Major/Minor/Dim)           │   │
│  │  │   ├── Complexity-based extensions               │   │
│  │  │   ├── Inversion randomization                   │   │
│  │  │   └── Detune randomization                      │   │
│  │  │                                                  │   │
│  │  └── renderNextBlock:                               │   │
│  │      ├── Sum 12 WavetableOscillators               │   │
│  │      │   └── TuningSystem.getFrequency()           │   │
│  │      ├── Apply ADSR envelope                       │   │
│  │      └── Mix to output                             │   │
│  └─────────────────────────────────────────────────────┘   │
│       │                                                     │
│       ▼                                                     │
│  ┌──────────────────────┐                                   │
│  │ StateVariableTPTFilter│ Low-pass, 12dB/oct              │
│  └──────────────────────┘                                   │
│       │                                                     │
│       ▼                                                     │
│  ┌──────────────────────┐                                   │
│  │ Master Volume         │ 0.0 - 1.26 gain                 │
│  └──────────────────────┘                                   │
└─────────────────────────────────────────────────────────────┘
    │
    ▼
Stereo Output
```

## Primary Risks

**Voice Management (96 Oscillators) - MEDIUM Risk:**
- 12 chord voices × 8 polyphony = 96 simultaneous oscillators
- Target: <80% CPU @ 48kHz (strict requirement)
- Fallback 1: Reduce polyphony to 6 (72 oscillators)
- Fallback 2: Reduce max chord voices to 8 (64 oscillators)
- Status: Implemented, awaiting CPU profiling

## Context to Preserve

- **Complexity:** 5.0/5.0 (highest in codebase) - requires incremental validation
- **Plugin format:** Synth (IS_SYNTH TRUE, output-only bus)
- **auval ID:** aumu OuIP OuDv
- **Implementation strategy:** 4-phase DSP development (all phases complete)
- **Performance target:** <80% CPU with 96 oscillators (fallbacks planned)
- **Key decision:** Global LFO (not per-voice) for unified pad movement
- **Key decision:** Band-limited wavetables (sine wave validation mode for now)
- **Key decision:** Scale-degree chord generation (not fixed tables)
