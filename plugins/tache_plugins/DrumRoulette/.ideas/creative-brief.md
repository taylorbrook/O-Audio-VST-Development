# DrumRoulette - Creative Brief

## Overview

**Type:** Instrument
**Core Concept:** Eight-slot drum sampler with folder-based randomization and mixer-style interface
**Status:** 💡 Ideated
**Created:** 2025-11-12

## Vision

DrumRoulette is a drum sampler designed around creative exploration through controlled randomization. Each of the eight sample slots connects to a folder on your computer, allowing you to randomize individual sounds or the entire kit while maintaining creative control through per-channel lock buttons. The interface resembles a mixing console, with eight vertical channel strips providing immediate access to essential drum processing controls.

The plugin emphasizes drum-specific parameter design: decay controls tighten or lengthen sounds without sustain, attack times are tailored for percussive shaping, and a tilt filter provides intuitive brightness control. Multi-output routing enables advanced production workflows, allowing each drum to be processed independently in your DAW.

## Parameters

### Global Controls
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Randomize All | Button | - | Randomize all unlocked slots simultaneously |

### Per-Slot Controls (×8)
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Folder Path | File Browser | Empty | Select folder containing audio samples |
| Randomize | Button | - | Pick random sample from assigned folder |
| Lock | Toggle | Off | Exclude slot from global randomization |
| Volume | Fader (-inf to +6dB) | 0dB | Channel level control |
| Decay | 10ms - 2s | 500ms | Envelope decay time (sustain always 0) |
| Attack | 0 - 50ms | 1ms | Envelope attack time for percussive shaping |
| Tilt Filter | -12dB to +12dB | 0dB | Brightness control (pivot at 1kHz) |
| Pitch | ±12 semitones | 0 | Pitch shift in semitones |
| Solo | Toggle | Off | Solo this channel |
| Mute | Toggle | Off | Mute this channel |

## UI Concept

**Layout:** Eight vertical channel strips arranged horizontally, resembling a mixing console
**Visual Style:** Vintage hardware mixer with brushed metal texture, rack-mount styling, and skeuomorphic details (inspired by LushVerb aesthetic)
**Key Elements:**
- LED-style trigger indicators per channel (velocity-sensitive brightness)
- Horizontal button row per channel: Folder | Randomize | Lock (SVG icons, no emojis)
- Sample name display (monospace font for hardware aesthetic)
- Centered vertical faders with horizontal Solo/Mute buttons below
- Global randomize button prominently placed in master section
- Master volume knob and LED output meter
- Brushed metal texture overlay and rack mounting holes
- Ridged knob texture for tactile realism

## Use Cases

- **Rapid beat sketching:** Quickly explore variations by randomizing drum kits from curated sample folders
- **Live performance:** Lock favorite sounds while improvising with randomization on other slots
- **Sample discovery:** Browse large sample libraries by rapidly randomizing through folders
- **Parallel processing:** Route individual drums to separate DAW channels for advanced mixing and effects
- **Creative constraints:** Use limited sample folders with randomization to inspire unexpected combinations

## Inspirations

- Mixing console workflow (SSL, Neve) for interface layout
- Battery/Geist for multi-slot drum sampling approach
- Randomization features in modern production tools
- Hardware drum machines with per-voice outputs

## Technical Notes

**Audio Engine:**
- True random file selection from folders (any file each time)
- Recursive folder scanning (includes subfolders)
- Support for WAV, AIFF, MP3, and AAC formats
- Sample playback with ADSR envelope (sustain fixed at 0 for decay-focused control)
- Tilt filter centered at 1kHz for intuitive brightness/darkness control
- Pitch shifting ±12 semitones

**MIDI Implementation:**
- Eight slots mapped to C1-G1 (chromatic)
- Standard MIDI velocity response

**Audio Routing:**
- Automatic multi-output routing (no user control)
- Stereo main output (mix of all channels)
- 8 individual stereo outputs (Slot 1→Out 1-2, Slot 2→Out 3-4, ..., Slot 8→Out 15-16)
- 18 total outputs (2 main + 16 individual)

**File Handling:**
- Error messages when folders contain no valid audio files
- Manual folder selection each session (no persistence)
- Folder path stored with project for recall

**Randomization Logic:**
- Per-channel randomize buttons
- Global randomize button (affects all unlocked slots)
- Lock buttons exclude specific slots from global randomization
- True random selection (no shuffle/cycle behavior)

## Design Evolution

**Date:** 2025-11-12
**Stage:** UI mockup design (v1→v4)

**Changes from original vision:**

1. **Visual aesthetic evolved:**
   - Original: "Clean, functional mixer aesthetic"
   - Final: "Vintage hardware mixer with brushed metal texture"
   - Reason: User requested vintage hardware aesthetic during mockup iteration, provided LushVerb as reference for skeuomorphic design approach

2. **Output Routing simplified:**
   - Original: User-controllable dropdown (Main or Individual Out 1-8)
   - Final: Automatic routing (each slot routes to consecutive stereo outputs)
   - Reason: User requested automatic routing for streamlined workflow
   - Impact: Removed 8 parameters, reduced complexity

3. **Added polish features:**
   - Master volume knob (UI convenience, not saved as parameter)
   - LED output meter (visual feedback for master output level)
   - Brushed metal texture overlay, rack mounting holes (hardware aesthetic)
   - Ridged knob texture (tactile realism)

4. **Layout refinements:**
   - Buttons reorganized into horizontal row (Folder | Randomize | Lock) with SVG icons
   - Faders centered in channel strips for visual balance
   - Solo/Mute buttons moved below faders (horizontal layout, mixer-style)
   - Symmetric horizontal padding (mathematical perfection, no dead space)

**Core concept preserved:** Eight-slot drum sampler with folder-based randomization and mixer-style interface remains intact. All original use cases supported.

**Final parameter count:** 73 parameters (reduced from ~81 due to automatic routing)

## Next Steps

- [x] Create UI mockup (v4 finalized 2025-11-12)
- [ ] Start implementation (`/implement DrumRoulette`)
