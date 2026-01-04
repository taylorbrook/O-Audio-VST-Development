# Words - Creative Brief

## Overview

**Type:** Utility (MIDI Sequencer)
**Core Concept:** Text-to-MIDI polymetric sequencer where typed words become evolving musical loops
**Status:** 💡 Ideated
**Created:** 2025-11-13

## Vision

Words transforms language into music through algorithmic mapping. Users type lines of text into a blank canvas, and each line becomes an independent MIDI loop that plays continuously. The magic emerges from polymetric phasing: because each word has different letter values, syllable counts, and spacing, the loops drift in and out of phase, creating constantly evolving generative patterns.

The plugin bridges the gap between text-based composition and musical sequencing. Instead of programming MIDI notes manually, users explore musical ideas by typing phrases, sentences, or poetic fragments. The deterministic letter-to-pitch mapping means the same text always produces the same pattern, but combining multiple lines creates emergent complexity through phase relationships.

Designed for generative composition, ambient textures, and live performance where evolving patterns are desired. Works seamlessly with any DAW or hardware synth via standard MIDI output.

## Core Mapping System

### Letter Analysis

Every letter has a numeric value (A=1, B=2...Z=26). Each word is analyzed for:
- **Total letter sum** (all letters added together)
- **First and last letter values**
- **Syllable count**
- **Letter sequence properties** (vowel/consonant patterns, unique letters)

### Musical Translation

**Pitch:**
- Letter sum % 7 → scale degree (1-7)
- Mapped to selected scale (major, minor, modes, pentatonic, etc.)
- Quantized in real-time to current scale/root

**Velocity:**
- Absolute difference between first and last letter values
- Scaled to 50-127 range
- Modified by Bloom filter (velocity variation)

**Note Duration:**
- Syllable count = duration in sixteenth notes
- Clamped to 1-4 sixteenths

**Octave Placement:**
- Comparing consecutive word letter sums:
  - Difference ≤20: base octave
  - Difference 21-40: +1 octave
  - Difference >40: +2 octaves

**Rhythmic Spacing:**
- Rest between words = (average of two word sums) % 4 + 1 sixteenths
- Creates variable gaps (1-4 sixteenths) based on surrounding words

## User Experience

### Typing Interface

Users see a blank text canvas. Type a line, press Enter, and that line immediately starts looping as a MIDI pattern. A cursor bounces from word to word showing what's currently playing.

### Polymetric Layering

Each new line added creates another simultaneous loop (up to 8 lines). Different word counts and syllable patterns create different loop lengths:
- 3-word line might loop every 12 sixteenths
- 5-word line might loop every 20 sixteenths
- They phase against each other, realigning at different intervals

**Example:**
- Line 1: "the sky falls" (3 words, short syllables) = quick repeating pattern
- Line 2: "wandering through empty corridors" (4 words, longer syllables) = slower, longer loop
- Result: 3:4 polyrhythm that shifts as individual words align and separate

### Visual Feedback

- Each line displays its text with cursor highlighting current word
- Lines can be muted/unmuted individually
- Each line gets its own MIDI channel (indicated by color)
- 8 simultaneous lines maximum (focused layering)

### Live Editing

Text can be edited while playing. Pattern regenerates instantly on next note, allowing real-time morphing of musical material.

### Loop Timing

Global sync mode: all loops align to main grid. New loops wait for downbeat to start, maintaining phase-lock to DAW clock.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Transpose | -12 to +12 scale degrees | 0 | Global pitch shift (stays in-scale) |
| Transpose Preset 1-8 | MIDI keyswitch trigger | C3-G3 | 8 MIDI notes trigger stored transpose values |
| Density | 0-100% | 100% | Random probability gate (note dropping) |
| Dynamics | 0-200% | 100% | Velocity scaling (squash/expand range) |
| Bloom Amount | 0-100% | 0% | Harmonic spreading intensity |
| Bloom Pitch | On/Off | On | Shift notes ±1 scale degree |
| Bloom Velocity | On/Off | On | Add random velocity offset to bloomed notes |
| Tempo | 20-300 BPM | 120 BPM | Master clock (synced to DAW) |
| Scale | Choice (12+ options) | Major | Musical scale (major, minor, modes, pentatonic) |
| Root | C-B | C | Root note of scale |
| Base Octave | -2 to +2 | 0 | Starting point for octave calculations |

## UI Concept

**Layout:**
- Top section: 8 text line slots with play/mute/channel indicators
- Each line shows typed text with animated cursor
- Center: Real-time controls (Transpose slider with preset recall, Density, Dynamics, Bloom)
- Bottom: Musical foundation (Scale selector, Root note, Base octave, Tempo display)

**Visual Style:**
- Minimal, text-focused interface
- Color-coding for MIDI channels (8 colors)
- Monospaced font for text lines (typewriter aesthetic)
- Cursor animation showing playback position per line

**Key Elements:**
- Text input fields (8 slots)
- MIDI channel indicators (color badges)
- Transpose vertical slider with 8 keyswitch positions
- Transpose exclusion toggles (exclude specific lines from transposition)
- Parameter knobs/sliders (Density, Dynamics, Bloom controls)
- Scale/root/octave selectors
- Line mute buttons

## Use Cases

- **Generative ambient composition:** Type abstract phrases, let patterns evolve over time
- **Live performance:** Edit text in real-time to morph musical material
- **Algorithmic exploration:** Discover how different words create different melodic contours
- **Polyrhythmic layering:** Exploit phase relationships between loops of different lengths
- **Lyric-based composition:** Use song lyrics or poetry as source material for melodic patterns
- **Educational tool:** Visualize relationship between language structure and musical rhythm

## Inspirations

- Brian Eno's generative music systems
- Polymetric sequencers (Elektron, Polyend)
- Constraint-based composition (Oulipo, procedural art)
- Text-to-sound mapping experiments
- Live coding aesthetics (TidalCycles, Sonic Pi)

## Technical Notes

### MIDI Implementation

- Sends standard MIDI (note on/off, clock, start/stop)
- Each line plays on dedicated MIDI channel (1-8)
- DAW transport sync only (no internal transport)
- All timing based on sixteenth note grid
- Quantized to selected scale in real-time

### Text Parsing

- Accepts alphabetic characters only (A-Z, case-insensitive)
- Non-alphabetic characters filtered out
- Words separated by whitespace
- Syllable counting algorithm for duration mapping

### Scale Changes

When scale or root note changes:
- Kill all currently playing notes (send note-offs)
- Clear active note state
- Restart patterns with new scale mapping

### Project Management

Save/load projects preserve:
- All typed text lines
- MIDI channel assignments
- Active/muted states per line
- Transpose preset values (8 keyswitch mappings)
- Lines excluded from transposition
- Current scale, root, base octave settings

## Next Steps

- [ ] Create UI mockup (`/dream Words` → option 3)
- [ ] Start implementation (`/implement Words`)
