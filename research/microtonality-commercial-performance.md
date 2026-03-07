---
title: "Microtonality in VST Plugins: Commercial Products, Performance, and Modern Approaches"
created: 2026-01-09
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Commercial product analysis and modern approaches for microtonal VST plugins, covering MTS-ESP integration, MPE maturation, MIDI 2.0 emergence, performance optimization, UI/UX design patterns, and testing methodologies."
domain: market-research
type: research
keywords:
  - microtonality
  - mts-esp
  - mpe
  - midi-2
  - commercial-analysis
  - tuning-systems
stages: [0]
agents: [research]
---

# Microtonality in VST Plugins: Commercial Products, Performance, and Modern Approaches

**Research Date:** January 2026
**Scope:** Commercial product analysis, MTS-ESP integration, performance optimization, UI/UX patterns, modern standards (MIDI 2.0, MPE), testing methodologies

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Commercial Product Analysis](#commercial-product-analysis)
3. [MTS-ESP Integration Guide](#mts-esp-integration-guide)
4. [Performance Optimization](#performance-optimization)
5. [UI/UX Design Patterns](#uiux-design-patterns)
6. [Modern Approaches (2020-2026)](#modern-approaches-2020-2026)
7. [Testing and Validation](#testing-and-validation)
8. [Recommendations](#recommendations)
9. [Sources](#sources)

---

## Executive Summary

The microtonality landscape in VST plugins has evolved significantly since 2020, driven by three key developments:

1. **MTS-ESP Standardization**: ODDSound's MTS-ESP protocol has become the de facto standard for cross-plugin microtuning, adopted by major manufacturers including Arturia, u-he, and Native Instruments.

2. **MPE Maturation**: MIDI Polyphonic Expression provides per-note pitch control, enabling real-time microtonal expression without dedicated tuning tables.

3. **MIDI 2.0 Emergence**: Native per-note controllers and 16-bit resolution promise to simplify microtonal implementations, though DAW adoption remains in progress.

**Key Finding**: For new plugin development, implementing both MTS-ESP client support and MPE compatibility provides the broadest user reach while maintaining future compatibility with MIDI 2.0.

---

## Commercial Product Analysis

### Feature Comparison Table

| Product | Scala SCL | TUN Files | MTS-ESP | MPE | Per-Note Pitch | Notes per Octave | Price Point |
|---------|-----------|-----------|---------|-----|----------------|------------------|-------------|
| **Surge XT** | Yes | Yes | Master + Client | Yes | Yes | Unlimited | Free/OSS |
| **Pianoteq** | Yes + KBM | No | Incoming MTS | No | No | Unlimited | $149-599 |
| **u-he Zebra** | No | Yes | Client | No | No | 128 | $199 |
| **u-he Diva** | No | Yes | Client | No | No | 128 | $189 |
| **u-he ACE** | No | Yes | Client | No | No | 128 | $99 |
| **Omnisphere** | No | Yes | No | No | No | 48/octave | $499 |
| **Kontakt** | Via Script | Via Script | No | Limited | Via KSP | Varies | $399-649 |
| **Arturia V Collection** | Yes (Pigments) | Yes | Client | Yes (v9+) | Yes | 128 | $599 |
| **Bitwig (DAW)** | Via Micro-pitch | Via Micro-pitch | No | Yes | Yes | Unlimited | $99-399 |
| **Madrona Aalto** | Yes | No | No | Yes | Yes | 128 | $99 |
| **Madrona Kaivo** | Yes | No | No | Yes | Yes | 128 | $129 |
| **NI Reaktor** | Manual/User Lib | Manual | No | Yes | Yes | User-defined | $199-599 |
| **NI Absynth** | Built-in | Built-in | No | No | No | Extensive | Discontinued |

### Detailed Product Analysis

#### Surge XT (Open Source Reference Implementation)

**Architecture**: Dual-scene hybrid synthesizer built on JUCE framework with modular codebase.

**Tuning Implementation**:
- Full Scala SCL/KBM format support
- Integrated Tuning Editor with analysis tools
- Can act as both MTS-ESP master AND client (since v1.2)
- Non-monotonic intonation system support
- Two tuning application modes:
  - "Apply tuning at MIDI input" (default)
  - "Apply tuning after modulation"

**Source Code**: Available at [github.com/surge-synthesizer/surge](https://github.com/surge-synthesizer/surge)

**Key Files**:
- `src/common/` - Engine code including tuning logic
- `src/surge-xt/gui/` - Tuning Editor UI
- SSE2 hand-coded DSP for performance

**Developer Resources**: The Surge Synth Team provides an open-source tuning library for implementing SCL/KBM in other instruments.

#### u-he Products (Zebra, Diva, Repro, ACE, Bazille)

**Implementation**:
- Native TUN file format support
- Files stored in `~/u-he/Tunefiles/`
- Global/FX section activation required
- MTS-ESP client support added (all products)
- No native Scala support (requires conversion)

**Limitations**:
- Users must maintain separate tuning file archives per plugin
- No direct SCL loading (convert via Scala app or Scale Workshop)
- 128-note tuning table limit

**Best Practice**: Use MTS-ESP for dynamic tuning or convert SCL to TUN format using Scale Workshop.

#### Pianoteq (Physical Modeling)

**Implementation**:
- One of the most complete Scala implementations
- Direct SCL + KBM file loading from UI
- Built-in temperaments: Pythagorean, Zarlino, Meantone, Werckmeister III, Equal, Flat
- Real-time MTS (MIDI Tuning Standard) message support
- Tuning modeling options: "Full rebuild" vs "String tension"

**Unique Features**:
- Physical modeling responds to tuning changes naturally
- Octave stretching options
- Master pitch range: 420-460 Hz

**Recommendation**: Use "Flat" temperament as baseline for custom Scala tunings.

#### Omnisphere (Spectrasonics)

**Implementation**:
- TUN file import only
- Per-Part scale assignment (8 simultaneous scales in Multi)
- Scale hierarchy: Global > Multi > Patch
- Master tuning range: 420-460 Hz

**Built-in Scales**: Bohlen-Pierce, Darreg Genus, Ellis 24-tone, Partch 29-tone, African mallet instruments

**Limitations**:
- Maximum ~48 tones per octave practical limit
- Extended tunings (31-EDO+) lose notes at bottom of range
- No MTS-ESP support
- Cannot play full range of notes in microtunings

#### Kontakt (Native Instruments)

**Implementation**: Entirely via KSP (Kontakt Script Processor)

**Key Function**: `change_tune($EVENT_ID, millicents, 0)` - tunes in millicents (1/1000 of a cent)

**Available Resources**:
- Orange Tree Samples Free Pro Microtuning Script
- Real-time tuning updates
- Automated tuning via MIDI CC

**Challenges**:
- Many commercial libraries are "12-locked"
- Requires scripting knowledge for full microtuning
- Some libraries ignore tuning scripts entirely

**Recommendation**: For microtonal Kontakt work, verify library compatibility or build custom instruments.

#### Arturia V Collection

**Evolution**:
- V Collection 8.1: Initial MTS-ESP support (OB-Xa V, Vocoder V, Emulator II V, Jup-8 V, Jun-6 V)
- V Collection 9: Expanded to 16+ instruments
- 2025 Update: Pigments, Augmented series, Synthx V, Mini V, Acid V, Buchla Easel V added

**Pigments Specific**:
- Direct Scala (.scl) and .tun file import
- Built-in alternative scales
- Continuous and note-on retuning modes

#### Bitwig Studio (DAW)

**Micro-pitch Device** (v3.1+):
- Note FX for real-time stream retuning
- 30+ included presets (historical temperaments, cultural scales, artist scales)
- Works with:
  - Built-in instruments
  - MPE-ready VST plugins
  - Hardware via CV

**Expression Modulator**:
- Per-note modulation for any plugin parameter
- Works with standard MIDI keyboards (velocity/mod wheel)
- Full MPE controller support (Linnstrument, Seaboard, etc.)

#### Madrona Labs (Aalto, Kaivo)

**Implementation**:
- Scala file support with caveats
- MPE support (v1.7+)
- Soundplane/OSC integration

**Known Issue**: Default tuning interpretation differs from Scala standard:
- Aalto/Kaivo: Root note = A4, reference = A4=440Hz
- Scala standard: Root note = C4, reference = C4=261.625565Hz
- **Solution**: Use matching .kbm file to override default behavior

**MPE Settings**:
- Aalto pitch bend range: +/-48 semitones
- Kaivo pitch bend range: +/-24 semitones

#### Native Instruments Reaktor

**Implementation**: Modular/user-built

**Built-in Macros**:
- Equal-tempered tuning (set divisions of octave)
- Formula: `(note - root) * (12 / divisions) + root`

**User Library Resources**:
- Microtune 1024
- Wendy Carlos alpha/beta scales
- Bohlen Pierce scale
- Microtonal Games sequencer

**Limitations**:
- No direct tuning file import
- Manual value entry required for custom scales
- Blocks support limited to 44 equal divisions

---

## MTS-ESP Integration Guide

### Architecture Overview

MTS-ESP (MIDI Tuning Standard - Extrasensory Perception) enables automatic tuning synchronization between plugins in a DAW session.

```
┌─────────────────┐         ┌─────────────────┐
│  MTS-ESP MASTER │         │  MTS-ESP CLIENT │
│                 │         │                 │
│ - Defines tuning│────────>│ - Follows tuning│
│ - One per       │  IPC    │ - Unlimited     │
│   session       │         │   instances     │
│ - Can filter    │         │ - Can query     │
│   notes         │         │   continuously  │
│                 │         │   or at note-on │
└─────────────────┘         └─────────────────┘
```

### Master Plugin Requirements

1. **Registration**: Check for existing master before registering
2. **Tuning Definition**: Specify frequency for each MIDI note (0-127)
3. **Note Filtering**: Optionally specify unmapped keys
4. **IPC Support**: Handle inter-process communication for hosts with separate plugin processes
5. **Crash Recovery**: Provide re-initialization capability

**Library Location**:
- Windows 64-bit: `Program Files\Common Files\MTS-ESP`
- Windows 32-bit: `Program Files (x86)\Common Files\MTS-ESP`
- macOS: Included in plugin bundle

### Client Plugin Implementation

**Basic Integration** (typically <30 minutes):

```cpp
#include "libMTSClient.h"

// On initialization
MTSClient* client = MTS_RegisterClient();

// On note-on
double frequency = MTS_NoteToFrequency(client, noteNumber, -1);
bool shouldFilter = MTS_ShouldFilterNote(client, noteNumber, -1);

// On destruction
MTS_DeregisterClient(client);
```

**Recommended Features**:
1. Display MTS-ESP connection status in UI
2. Allow user choice: query at note-on only OR continuously during notes
3. Handle note filtering for keyboard maps with unmapped keys
4. Fallback to 12-TET when no master connected
5. Support MIDI Tuning Standard SysEx for local tuning table updates

### Performance Characteristics

- **Latency**: Zero (direct memory access via shared library)
- **CPU**: Negligible ("low CPU use" per ODDSound)
- **Memory**: Shared tuning table, minimal per-client overhead

### Available Masters

| Master | Features | Price |
|--------|----------|-------|
| MTS-ESP Mini | SCL/KBM/TUN/MTS SysEx loading | Free |
| MTS-ESP Master | Full editing, automation, multi-channel | $79 |
| Surge XT | Built-in tuning editor, can act as master | Free |
| Wilsonic MTS-ESP | Erv Wilson scale designs | Open source |
| Infinitone DMT | Advanced tuning design | Commercial |

---

## Performance Optimization

### Real-Time Pitch Calculation Strategies

#### 1. Lookup Tables (LUT)

**Advantages**:
- O(1) access time
- Predictable performance
- Simple implementation

**Disadvantages**:
- Cache pressure with large tables
- Memory bandwidth limitations
- "Very large lookup tables mess up the cache. RAM is really slow these days."

**Optimal Size**: 128-4096 entries with linear interpolation

**Implementation Pattern**:
```cpp
// Pre-compute 128 MIDI note frequencies
float tuningTable[128];
void initializeTuning(const ScalaFile& scala) {
    for (int i = 0; i < 128; i++) {
        tuningTable[i] = calculateFrequency(scala, i);
    }
}

// Real-time lookup with interpolation for pitch bend
float getFrequency(int note, float pitchBend) {
    float baseFreq = tuningTable[note];
    // Apply pitch bend with table interpolation
    return baseFreq * pitchBendFactor(pitchBend);
}
```

#### 2. On-Demand Calculation

**Advantages**:
- No memory overhead
- Supports unlimited scales
- Better for infrequently used notes

**Disadvantages**:
- Variable CPU per note
- Potential cache misses
- More complex for just intonation ratios

**Best Use Case**: Systems with complex, frequently-changing tunings

#### 3. Hybrid Approach (Recommended)

```cpp
class TuningCache {
    std::array<float, 128> baseTable;      // Pre-computed at scale load
    std::array<float, 128> modulatedTable; // Updated when tuning parameters change
    bool tableValid = false;

    float getFrequency(int note) {
        if (!tableValid) rebuildTable();
        return modulatedTable[note];
    }

    void onParameterChange() {
        tableValid = false;  // Lazy rebuild
    }
};
```

### Voice Management Efficiency

**Per-Voice Tuning State**:
- Store tuned frequency at note-on (for note-on-only tuning mode)
- OR store reference to tuning source for continuous updates
- Minimize per-sample calculations

**Memory Considerations**:
- Voice base memory: ~10-20 KB typical
- Per-instance overhead: ~500 KB
- Formula: `Total = base + (instances * 500KB)`

**CPU Utilization Guidelines**:
- Stay below 85% total CPU
- "When CPU utilization exceeds 85%, performance degrades rapidly"
- Profile with worst-case polyphony (16+ voices, complex tuning)

### Caching Strategies

1. **Scale Cache**: Keep recently used scales in memory
2. **Frequency Cache**: Pre-compute for all 128 MIDI notes at scale load
3. **Ratio Cache**: Store just intonation ratios as doubles to avoid repeated division
4. **Interpolation Cache**: Pre-compute pitch bend tables for common bend ranges

### DSP Optimization Tips

**Lookup Table Best Practices**:
- Use linear interpolation for values between table entries
- Maintain phase continuity when changing frequencies mid-note
- Avoid discontinuities that cause "clicking"

**Polynomial Approximation** (alternative to LUT for sine):
- "Polynomial approximation is the best way to do sine"
- Straightforward to vectorize with SIMD
- Better cache behavior than large tables

**SIMD Considerations**:
- Align data structures on 16-byte boundaries (SSE2) or 32-byte (AVX)
- Process voices in groups of 4 or 8
- Surge XT uses "hand-coded SSE2 implementations of most activities"

---

## UI/UX Design Patterns

### Scala File Loading Interface

**Best Practices**:

1. **Dual File Support**: Allow loading SCL (scale) and KBM (keyboard mapping) separately
2. **Preset Browser**: Organize tunings by category (historical, cultural, equal temperaments)
3. **Recent Files**: Quick access to recently used tunings
4. **Preview**: Play test notes while browsing scales
5. **Validation**: Show warnings for files with parsing issues

**Pianoteq Example** (considered best-in-class):
- Direct UI loading for both SCL and KBM
- Built-in temperament dropdown
- "+" button for file browser
- Clear temperament name display

**Scale Workshop Pattern**:
- Web-based tuning design
- Real-time playback with web synth
- Export to multiple formats (SCL, KBM, TUN, MTS SysEx)

### Visual Tuning Editors

**Surge XT Tuning Editor Features**:
- Interactive keyboard visualization
- Cent deviation display per note
- Scale analysis tools
- Export functionality
- Built-in scale collection

**Essential Visualizations**:
1. **Keyboard View**: Show pitch deviation from 12-TET per key
2. **Interval Matrix**: Display relationships between scale degrees
3. **Frequency Spectrum**: Show harmonic relationships
4. **Cent Ruler**: Visual representation of scale structure

### Keyboard Mapping Visualization

**Display Elements**:
- Unmapped keys (grayed out or hidden)
- Scale degree numbers
- Cent offsets from reference
- Octave boundaries

**Interactive Features**:
- Click to hear note
- Drag to reassign scale degrees
- Shift-click to set reference pitch

### Live Tuning Controls

**Essential Parameters**:
1. **Reference Pitch**: A4 frequency (default 440 Hz, range 415-466 Hz common)
2. **Root Note**: Starting note for scale mapping
3. **Octave Stretch**: For piano/physical modeling
4. **Real-time Modulation**: Tuning amount as automatable parameter

**Orange Tree Samples Script Features**:
- Real-time tuning update during adjustment
- MIDI CC automation for "tuning amount"
- Equal temperament vs pure tuning baseline selection
- Three decimal place cent input

### Preset Management

**Organization Structure**:
```
Tunings/
├── Equal Temperaments/
│   ├── 12-TET (Standard)
│   ├── 19-TET
│   ├── 31-TET
│   └── ...
├── Historical/
│   ├── Werckmeister III
│   ├── Kirnberger III
│   └── Meantone (1/4 comma)
├── Just Intonation/
│   ├── 5-limit JI
│   ├── 7-limit JI
│   └── Harry Partch 43-tone
├── Cultural/
│   ├── Arabic Maqam
│   ├── Gamelan Slendro
│   └── Gamelan Pelog
└── User/
    └── [Custom scales]
```

**Bitwig Presets Example**: 30+ categorized presets including historical (Kirnberger, Werckmeister, Broadwood), cultural (China, Java), and artist (Ives, Partch, Carlos).

---

## Modern Approaches (2020-2026)

### MIDI 2.0 Per-Note Controllers

**Standardization Progress**:
- November 2017: MPE specification released (AMEI)
- January 2018: MPE officially adopted by MMA
- June 2020: USB transport completed
- May 2024: DAW Working Group summit (Native Instruments Berlin)
- November 2024: Ethernet Network transport completed
- May 2025: DAW Working Group meeting (Ableton Berlin)

**Key Improvements**:
- **16-bit velocity** (vs 7-bit in MIDI 1.0)
- **Per-note pitch bend** native (no channel tricks)
- **Per-note controllers** for vibrato, modulation
- **Bidirectional communication** (device discovery, profiles)

**Current Adoption**:
- DAWs: Cubase (partial), MultitrackStudio
- Controllers: KORG Keystage, Native Instruments S-Series (hardware ready)
- Plugins: Limited native support, MPE bridge common

**Open Source Initiative**: MIT-licensed "MIDI-CI helper" toolkit in development for plugin format translation.

### MPE (MIDI Polyphonic Expression)

**How It Works**:
- Each note assigned to separate MIDI channel
- Channel 1 (or 16): Global messages (preset changes, pedals)
- Channels 2-16: Individual note data
- Maximum 15-note polyphony with full expression

**Mode 3 Enhancement**: "Poly Mode" allows channel polyphony when notes exceed available channels.

**Compatible Controllers**:
- Haken Continuum Fingerboard
- ROLI Seaboard
- Roger Linn Design Linnstrument
- Sensel Morph
- Eigenharp

**Plugin Support** (2025):
- Synapse Dune 3 (v3.64+)
- Most Bitwig instruments
- U-he products (limited)
- Arturia V Collection 9+
- Surge XT

**JUCE Implementation**:
```cpp
// Key MPE classes
MPESynthesiser
MPEInstrument
MPENote
MPEValue
SmoothedValue  // For glide smoothing
```

### Web Audio API Microtonality

**Capabilities**:
- Arbitrary frequency specification per note
- Real-time synthesis in browser
- No plugin installation required

**Key Tools**:

**Scale Workshop** ([github.com/SeanArchibald/scale-workshop](https://github.com/SeanArchibald/scale-workshop)):
- Design microtonal scales in browser
- Export to VST formats (SCL, KBM, TUN, MTS SysEx)
- Built-in isomorphic keyboard
- MIDI output with per-channel pitch bend

**Tone.js Framework**:
- DAW-like features in browser
- Prebuilt synths and effects
- Note specification as frequency or pitch-octave notation
- High-performance building blocks

**Limitations**:
- Higher latency than native plugins
- Limited polyphony
- Browser compatibility variations

### Machine Learning Applications

**Current Research Areas**:

1. **Synthesizer Parameter Prediction**:
   - InverSynth: STFT spectrograms + 2D CNN -> synth parameters
   - End-to-end audio -> parameter prediction

2. **Adaptive Tuning**:
   - Vocal control of synthesis with unsupervised ML
   - Perceptual mapping (hide synth-specific parameters)
   - Real-time timbre matching

3. **Reinforcement Learning for Music**:
   - RL-Tuner, Bach2Bach: Deep Q-Learning for fine-tuning
   - RaveForce: Audio rendering in RL pipeline
   - SMART: Aesthetic reward for symbolic music

4. **Real-Time Integration** (SuperCollider + ML):
   - Neural timbre transfer
   - DDSP-CNN-Tiny: Lightweight CNN for real-time CPU performance

**Future Applications for Microtuning**:
- Automatic scale suggestion based on audio analysis
- Adaptive just intonation tracking
- Style-matched tuning recommendations

---

## Testing and Validation

### Tuning Accuracy Verification

**Frequency Measurement Tools**:

1. **DDMF PluginDoctor**:
   - Double-precision FFT engine
   - Harmonic analysis with sinusoidal input
   - THD/THD+N calculation
   - Per-channel stereo analysis

2. **MathAudio THD Meter**:
   - Harmonics h1-h5 measurement
   - Test frequencies 20-10000 Hz
   - Matched filter noise reduction

3. **Melda Analyser**:
   - Peak frequency pitch tracking
   - Polyphonic material support
   - Spectrograph visualization

4. **Voxengo Span** (Free):
   - Real-time FFT spectrum analyzer
   - Harmonic response checking
   - Aliasing detection

**Testing Methodology**:

```
1. Generate test signal: sine wave at -18dBFS
2. Use 997Hz or 1kHz (997Hz shows aliasing better)
3. Measure fundamental frequency accuracy
4. Check for harmonic distortion
5. Verify against expected frequency table
```

**Cent Accuracy Standard**:
- Typical target: +/- 0.1 cents
- Acceptable: +/- 1 cent
- Perceptible difference: ~5 cents

### ABX Testing Protocol

For subjective comparison of tuning implementations:

1. Prepare reference (A) and test (B) audio
2. Generate random X (either A or B)
3. Listener identifies X without knowing source
4. Statistical significance requires multiple trials

### Automated Testing

**Unit Tests**:
- Frequency calculation accuracy per note
- Scale parsing correctness
- Keyboard mapping validity
- Edge cases (note 0, note 127, extreme pitch bend)

**Integration Tests**:
- MTS-ESP master/client communication
- Tuning file loading/saving round-trip
- Parameter automation
- State recall after preset save/load

**Performance Tests**:
- CPU usage per voice with tuning active
- Latency measurement
- Memory allocation during scale changes

### User Acceptance Patterns

**Common User Expectations**:
1. Instant tuning changes (no audible glitch)
2. Correct behavior with pitch bend
3. Consistent tuning across octaves
4. Accurate import of standard formats
5. Visual feedback of current tuning state

**Known Problem Areas**:
- Scala root note interpretation (C4 vs A4)
- Reference pitch handling (440 Hz vs 261.625565 Hz for C4)
- Pitch bend interaction with tuning tables
- Note-off tuning for release tails

---

## Recommendations

### For New Plugin Development

1. **Primary Implementation**: MTS-ESP client support
   - Broadest ecosystem compatibility
   - Minimal development effort (<30 minutes)
   - Zero runtime cost when not used

2. **Secondary Implementation**: Direct Scala SCL/KBM loading
   - Industry-standard format
   - Maximum user flexibility
   - Reference: Pianoteq implementation

3. **Expression Support**: MPE compatibility
   - Per-note pitch control
   - Future-proof for MIDI 2.0
   - Use JUCE MPESynthesiser classes

4. **Performance**: Hybrid tuning cache
   - Pre-compute 128-note table at scale load
   - Lazy rebuild on parameter change
   - Avoid large (>4096 entry) lookup tables

### Architecture Decisions

| Decision Point | Recommendation | Rationale |
|----------------|----------------|-----------|
| Tuning storage | 128-entry float table | Balance of precision and cache efficiency |
| Update timing | Note-on (with continuous option) | User preference varies |
| File formats | SCL + KBM + TUN | Maximum compatibility |
| Pitch bend | Apply after tuning lookup | Matches user expectation |
| Reference pitch | User-configurable, default 440 | Standard but allow historical tunings |

### UI/UX Priorities

1. **Essential**: Tuning file loading with preview
2. **Important**: Visual keyboard mapping display
3. **Nice-to-have**: Built-in tuning editor
4. **Future**: MTS-ESP master capability

### Testing Checklist

- [ ] All 128 MIDI notes produce correct frequency
- [ ] Pitch bend applies correctly over tuning
- [ ] Scale files load without errors
- [ ] State saves and restores correctly
- [ ] MTS-ESP connection status visible
- [ ] CPU usage acceptable at full polyphony
- [ ] No audio glitches on tuning change

---

## Sources

### Official Documentation
- [Surge XT User Manual](https://surge-synthesizer.github.io/manual-xt/)
- [Surge Synth Team Tuning Guide](https://surge-synthesizer.github.io/tuning-guide/)
- [Modartt Pianoteq Documentation](https://www.modartt.com/user_manual?product=pianoteq&lang=en)
- [MTS-ESP GitHub Repository](https://github.com/ODDSound/MTS-ESP)
- [ODDSound MTS-ESP Master Manual](https://oddsound.com/dl/ODDSound_MTS_ESP_Suite_Master_Manual.pdf)
- [Spectrasonics Omnisphere Pitch Options](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/system-tune-index)
- [Kontakt KSP Reference Manual](https://www.native-instruments.com/fileadmin/ni_media/downloads/manuals/kontakt/KONTAKT_602_KSP_Reference_Manual.pdf)

### Community Resources
- [Xenharmonic Wiki - List of Microtonal Software Plugins](https://en.xen.wiki/w/List_of_Microtonal_Software_Plugins)
- [Sevish Microtonal Music Blog](https://sevish.com/category/microtonal-music/)
- [KVR Audio Forums - Microtuning Discussions](https://www.kvraudio.com/forum/)
- [Scale Workshop](https://github.com/SeanArchibald/scale-workshop)
- [Scala Home Page](https://www.huygens-fokker.org/scala/)

### Standards and Specifications
- [MIDI.org - Microtuning Article](https://www.midi.org/midi-articles/microtuning-and-alternative-intonation-systems)
- [MPE Specification PDF](https://d30pueezughrda.cloudfront.net/campaigns/mpe/mpespec.pdf)
- [MIDI 2.0 Developer Resources](https://midi.org/category/https-midi-org-information-for-midi-2-0-developers)

### Developer Resources
- [JUCE MPE Tutorial](https://juce.com/tutorials/tutorial_mpe_introduction/)
- [Surge XT Source Code](https://github.com/surge-synthesizer/surge)
- [Surge Architecture Documentation](https://github.com/surge-synthesizer/surge/blob/main/doc/Surge%20Architecture.md)
- [Orange Tree Samples Free Pro Microtuning Script](https://www.orangetreesamples.com/blog/free-pro-microtuning-script)

### Testing Tools
- [DDMF PluginDoctor](https://ddmf.eu/plugindoctor/)
- [MathAudio THD Meter](https://mathaudio.com/thd-meter.htm)
- [REW Room Acoustics Software](https://www.roomeqwizard.com/)

### Product Pages
- [u-he Zebra Legacy](https://u-he.com/products/zebra-legacy/)
- [ODDSound MTS-ESP Suite](https://oddsound.com/mtsespsuite.php)
- [Bitwig Microtuning Guide](https://www.bitwig.com/learnings/microtuning-in-bitwig-studio-77/)
- [Madrona Labs Aalto](https://madronalabs.com/)
- [Arturia V Collection Updates](https://www.synthtopia.com/content/2021/07/13/arturia-updates-v-collection-with-microtuning-support-more/)

### Research and Articles
- [Web Audio API MDN Documentation](https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API)
- [Tone.js Framework](https://tonejs.github.io/)
- [Machine Learning for Audio Synthesis (ICML Workshop)](https://icml.cc/virtual/2022/workshop/13468)
- [Sound On Sound - ABC of MPE](https://www.soundonsound.com/sound-advice/mpe-midi-polyphonic-expression)

---

*Report generated January 2026. Product features and availability subject to change.*
