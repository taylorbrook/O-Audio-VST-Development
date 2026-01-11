# Ouaricon Marimba - Creative Brief

## Plugin Vision

A physically modeled marimba synthesizer with native microtonal support. The warmth and organic character of a real marimba, with pitch determined directly by the physical model - no pitch shifting or time-stretching. Minimal controls expose the essence of the instrument while offering deep microtonal customization.

## Core Identity

**Name:** Ouaricon Marimba
**Type:** Instrument (Synthesizer)
**Synthesis Method:** Modal Synthesis (Physical Modeling)
**Character:** Warm, organic, woody, resonant

## Sound Description

The sound should evoke a well-maintained concert marimba:
- **Attack:** Soft mallet strike with brief transient, not harsh
- **Body:** Warm fundamental with characteristic inharmonic overtones
- **Sustain:** Natural decay with high notes fading faster than low notes
- **Resonance:** Subtle body resonance adding depth and presence

The marimba's distinctive tone comes from its inharmonic modal structure - the overtones don't follow simple integer ratios, giving it that recognizable "woody" character distinct from bells or vibraphones.

## Microtonal Philosophy

Tuning is achieved through the physical model itself:
- Mode frequencies are calculated from the fundamental, not 12-TET
- Changing the fundamental changes all mode frequencies proportionally
- No pitch shifting artifacts - the model simply generates different frequencies
- Supports arbitrary tuning systems: Scala files, MTS-ESP, just intonation, EDOs

This is true microtonal synthesis - the physics determines pitch, not post-processing.

## Control Philosophy: Minimal & Musical

Only expose controls that make musical sense for a mallet player:

### Essential Controls (Always Visible)
1. **Mallet Hardness** - Soft/Medium/Hard character (affects attack brightness)
2. **Bar Material** - Tonal character from dark rosewood to bright synthetic
3. **Resonance** - Body coupling and sustain
4. **Tuning Source** - 12-TET / Scala File / MTS-ESP

### Tuning Controls (Expandable Panel)
- **Reference Pitch** - A4 frequency (default 440 Hz)
- **Scala File Loader** - Load .scl and .kbm files
- **MTS-ESP Status** - Connection indicator (auto-follows when master present)
- **Root Note** - Which MIDI note maps to scale degree 0

## Technical Foundation

### Physical Model Architecture

```
[Mallet Exciter] --> [Modal Resonator Bank] --> [Body Resonance] --> Output
                            |
                      Mode Frequencies
                      (from tuning table)
```

**Mallet Exciter:**
- Short noise burst shaped by hardness parameter
- Harder mallet = more high-frequency content in excitation
- Attack envelope ~5-20ms

**Modal Resonator Bank:**
- 8 resonant modes per voice (2nd-order IIR filters)
- Mode frequencies follow marimba bar physics
- Each mode has: frequency, amplitude, decay rate
- Fundamental frequency set by tuning system (Scala/KBM or MTS-ESP)

**Marimba Modal Ratios (approximate):**
```
Mode 1 (Fundamental): 1.00
Mode 2: ~3.93 (slightly flat double octave)
Mode 3: ~9.24 (varies with bar design)
Mode 4: ~16.65
Mode 5: ~26.3
Mode 6: ~38.2
Mode 7: ~52.4
Mode 8: ~68.9
```
*Note: Actual ratios will be refined during DSP implementation based on acoustic research*

**Body Resonance (Convolution IR):**
- Short impulse response (~50-100ms) of resonator tube
- Provides authentic spatial character and warmth
- Low CPU impact due to short IR length
- Could include 2-3 IR options for tonal variety

**Velocity Response:**
- Custom curve for expressive dynamics
- Affects both mallet hardness (excitation spectrum) and output level
- Soft playing = warmer tone, hard playing = brighter attack

### Microtonal Integration

**Tuning Priority:**
1. MTS-ESP (if master present) - real-time, DAW-wide
2. Scala file (if loaded) - file-based custom tuning
3. 12-TET fallback - standard Western tuning

**Implementation:**
```cpp
double getModeFrequency(int midiNote, int modeNumber) {
    // Get fundamental from tuning system (NOT 12-TET formula)
    double fundamental = tuningEngine.getFrequency(midiNote);

    // Apply marimba modal ratio
    return fundamental * marimbaModalRatios[modeNumber];
}
```

The key insight: the physical model parameters (mode frequencies) are set from the tuning table. The model doesn't know or care about 12-TET - it just generates the frequencies it's told.

## Target Users

1. **Microtonal composers** - Need instruments that play their tuning systems natively
2. **World music producers** - Gamelan, maqam, and other non-Western traditions
3. **Sound designers** - Organic mallet textures with unique tuning
4. **Minimalist composers** - Just intonation and pure intervals
5. **Ambient/experimental artists** - Unusual scales and timbres

## UI Vision (Finalized)

WebView-based interface with Ouaricon naturalist aesthetic.

**Window Size:** 600 x 400 pixels

**Visual Style:**
- Paper texture background
- Coral/tree botanical overlay (shifts right on tuning tab)
- Warm earth tones: browns (#8B7355), greens (#6B8E4E), cream (#F5E6D3)
- Serif typography (Garamond)
- Wooden knob aesthetic with radial patterns

**Layout - Tabbed Interface:**

### SOUND Tab (Quadrant Layout)
- **Top-Left:** Three 60px knobs - MALLET (Hardness), MATERIAL (Tone), RESONANCE (Body)
- **Top-Right:** Animated waveform display (260x120px)
- **Bottom-Left:** VELOCITY knob (60px) + velocity curve display (140x100px) with note trigger lines
- **Bottom-Right:** OUTPUT knob (60px) + VU meter (80px circular)

### TUNING Tab
- **Left Side:** Interval list (100x200px, shows 14 entries) + Pitch circle visualization (150px, lines radiating from center)
- **Right Side:** Mode buttons (12-TET, SCALA, MTS-ESP), A4 REF knob, scale name display
- **Bottom:** 2-octave keyboard visualization (400px, mapped notes in green)
- Context-sensitive controls: LOAD .SCL/.KBM buttons (Scala mode), MTS-ESP status indicator (MTS mode)

**Header:**
- Plugin title "Ouaricon Marimba"
- Preset browser with arrows and SAVE button

**Interactions:**
- Tab switching animates botanical overlay position
- Velocity curve updates in real-time with knob drag
- Note triggers display as vertical lines on curve
- Waveform animates on audio activity
- VU meter needle responds to output level
- Pitch circle redraws when tuning mode changes

## Performance Targets

- **Polyphony:** 16-24 voices
- **CPU per voice:** < 1%
- **Latency:** Suitable for live performance (< 10ms contribution)
- **Formats:** VST3, AU, Standalone

## Implementation Phases

**Stage 1 - Foundation:**
- CMake build system
- APVTS parameter infrastructure
- Basic synth voice architecture
- MIDI input handling

**Stage 2 - DSP:**
- 8-mode modal synthesis engine
- Mallet exciter with hardness control
- Convolution IR body resonance
- Material tonal shaping
- Custom velocity curve
- Tuning table infrastructure
- Scala/KBM file parser (Surge tuning library)
- MTS-ESP client integration
- Reference pitch control
- Nearest-pitch mapping for unmapped keys

**Stage 3 - GUI:**
- WebView UI implementation
- Mallet/Material/Resonance controls
- Tuning panel (file loader, MTS-ESP status)
- Velocity curve visualization
- Preset system

## Success Criteria

1. **Sounds like a marimba** - Recognizable acoustic character
2. **True microtonal** - Pitch from model, not pitch shifting
3. **Musical controls** - Every knob makes audible, useful changes
4. **CPU efficient** - 16+ voices at reasonable load
5. **Integrates seamlessly** - Works with MTS-ESP masters, DAW tuning

## Research References

This plugin draws from:
- `/troubleshooting/dsp-issues/physical-modelling-synthesis-complete-guide.md`
- `/research/microtonality-implementation-juce.md`
- `/research/microtonality-theory-formats.md`
- `/research-agent-3-physical-modelling-optimization.md`

## Design Decisions (Resolved)

1. **Mode count:** 8 modes - rich, authentic overtone structure
2. **Body model:** Convolution IR - realistic resonator tube response
3. **Note filtering:** Unmapped keys play nearest mapped pitch (musical, not purist)
4. **Velocity curve:** Custom curve for expressive dynamics

## Tuning File Support

Full Scala ecosystem support:
- **.scl files:** Scale definitions (intervals/ratios)
- **.kbm files:** Keyboard mappings (which keys play which scale degrees)
- Combined loading: SCL defines the scale, KBM defines how it maps to MIDI
- Reference pitch configurable (default A4 = 440 Hz)

This enables:
- Non-octave scales (Bohlen-Pierce, etc.)
- Partial keyboard mappings (e.g., white keys only)
- Custom reference frequencies
- Historical temperaments with specific key layouts

---

*Created: 2026-01-09*
*UI Finalized: 2026-01-09*
*Status: UI Design Complete - Ready for Implementation*
