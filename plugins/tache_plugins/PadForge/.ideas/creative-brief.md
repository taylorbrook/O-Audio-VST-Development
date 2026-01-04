# PadForge - Creative Brief

## Overview

**Type:** Synth
**Core Concept:** Randomizable pad generator with hidden parameters—click randomize to discover lush, evolving pad sounds
**Status:** 💡 Ideated
**Created:** 2025-11-14

## Vision

PadForge is a pad synthesizer designed for rapid sound discovery. The interface is intentionally minimal—just master volume, brightness, space (reverb), and a randomize button—while the engine contains dozens of hidden parameters. Users generate patches by clicking randomize, discovering unexpected and lush pad textures without the paralysis of endless knobs. A "Randomize Amount" knob controls how far from the current state the next randomization will go, enabling iterative refinement of good patches.

The sonic goal is to create pads that sound complete without external effects—rich, evolving, ambient textures powered by 3-oscillator subtractive synthesis with extensive internal modulation. Every patch includes movement from multiple LFOs, sample & hold randomization, and cross-modulation between parameters. Envelopes (amp + filter) are randomized within pad-appropriate ranges to ensure slow attacks and long releases suitable for ambient and experimental music.

## Parameters

### Visible Parameters (User-Facing)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Volume | 0.0 - 1.0 | 0.7 | Master output level |
| Brightness | 0.0 - 1.0 | 0.5 | Filter cutoff control (dark to bright) |
| Space | 0.0 - 1.0 | 0.3 | Algorithmic reverb amount |
| Randomize Amount | 0.0 - 1.0 | 0.8 | Scales how far randomization deviates from current state |

### Hidden Parameters (Randomized Internally)

**Oscillators (3 oscillators, 4 waveforms each: saw, square, triangle, sine):**
- Osc1 Waveform (choice: saw/square/tri/sine)
- Osc1 Detune (-50 to +50 cents)
- Osc1 Level (0.0 - 1.0)
- Osc2 Waveform (choice: saw/square/tri/sine)
- Osc2 Detune (-50 to +50 cents)
- Osc2 Level (0.0 - 1.0)
- Osc3 Waveform (choice: saw/square/tri/sine)
- Osc3 Detune (-50 to +50 cents)
- Osc3 Level (0.0 - 1.0)

**Filter (Single lowpass):**
- Filter Cutoff (internal, scaled by Brightness control) (20 Hz - 20 kHz)
- Filter Resonance (0.0 - 1.0)

**Envelopes (Amp + Filter, randomized within pad ranges):**
- Amp Attack (0.1 - 5.0 seconds)
- Amp Decay (0.1 - 3.0 seconds)
- Amp Sustain (0.3 - 1.0)
- Amp Release (0.5 - 8.0 seconds)
- Filter Attack (0.1 - 5.0 seconds)
- Filter Decay (0.1 - 3.0 seconds)
- Filter Sustain (0.3 - 1.0)
- Filter Release (0.5 - 8.0 seconds)

**Modulation (Multiple LFOs, random S&H, cross-mod):**
- LFO1 Rate (0.01 - 10 Hz)
- LFO1 Depth (0.0 - 1.0)
- LFO1 Target (choice: filter cutoff / osc pitch / osc mix / reverb)
- LFO2 Rate (0.01 - 10 Hz)
- LFO2 Depth (0.0 - 1.0)
- LFO2 Target (choice: filter cutoff / osc pitch / osc mix / reverb)
- Random S&H Rate (0.1 - 5 Hz)
- Random S&H Depth (0.0 - 1.0)
- Random S&H Target (choice: filter cutoff / osc pitch / osc mix / reverb)

**Effects:**
- Reverb Size (0.0 - 1.0, scaled by Space control)
- Reverb Damping (0.0 - 1.0)
- Reverb Modulation (0.0 - 1.0)

## UI Concept

**Window Size:** 400×350px (fixed, non-resizable)

**Layout:**
- Header: Plugin name "PadForge" (bold sans-serif, centered)
- Upper section: 3 knobs in horizontal row (Volume, Brightness, Space)
- Center section: Large RANDOMIZE button (perfectly centered) + Randomize Amount knob to the right
- Bottom section: Preset dropdown + Save/Load buttons in line

**Visual Style:**
- Earth tones with dusty/sandy texture (mixed natural palette)
- Subtle grain background overlay (fine sand texture, 15% opacity)
- Skeuomorphic vintage bakelite knobs with static lighting
- Raised tactile RANDOMIZE button with 3D depth
- Tactile, physical appearance ("want to reach out and touch it")

**Color Palette:**
- Sandy base (#c9a87c)
- Vintage bakelite brown (#8b6f47)
- Dark brown text (#3a2a1a)
- Warm accents and shadows for depth

**Key Elements:**
- Large "RANDOMIZE" button (160×60px, raised tactile, primary action) - randomizes hidden synthesis parameters only
- Volume knob (70px, bakelite style) - post-processing control, never randomized
- Brightness knob (70px, bakelite style) - post-processing control, never randomized
- Space knob (70px, bakelite style) - post-processing control, never randomized
- Randomize Amount knob (50px, bakelite style) - controls randomization range
- Preset dropdown + Save/Load buttons (bottom row)

## Use Cases

- Quickly generating unique pad sounds for ambient/experimental production
- Recording multiple randomized patches to find inspiration
- Creating evolving textures that don't need external effects
- Discovering unexpected timbres through controlled randomization
- Building a preset library of lush pads for various projects

## Inspirations

- Ambient/experimental sound design
- Generative music tools (controlled chaos)
- Classic subtractive synths (Juno, Prophet) for pad timbres
- "Happy accident" workflow—discovery through randomization

## Technical Notes

**Synthesis:** 3-oscillator subtractive with single lowpass filter

**Polyphony:** 8 voices

**Modulation architecture:** Multiple LFOs with selectable targets, sample & hold randomization, cross-modulation between parameters

**Randomization logic:**
- Randomize Amount knob scales deviation from current parameter values
- All hidden parameters randomized simultaneously on button press
- Envelopes constrained to pad-appropriate ranges (slow attacks, long releases)
- No musical scale constraints (this randomizes synth patch parameters, not note sequences)

**Reverb:** Algorithmic reverb integrated into synth engine

**Preset system:** User-saveable presets with dropdown recall

## Next Steps

- [ ] Create UI mockup (`/dream PadForge` → option 3)
- [ ] Start implementation (`/implement PadForge`)
