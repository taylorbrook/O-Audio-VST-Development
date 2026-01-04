# Context Extraction Guidelines

## Overview

Before designing UI, gather context from existing plugin documentation to inform design decisions.

## Files to Check

```bash
# Creative brief (plugin concept)
test -f "plugins/$PLUGIN_NAME/.ideas/creative-brief.md"

# Improvement notes (redesign context)
find "plugins/$PLUGIN_NAME/.ideas/improvements/" -name "*.md"

# Existing implementation (for redesigns)
test -d "plugins/$PLUGIN_NAME/Source/"
```

## What to Extract

### Plugin Type

Identifies UI conventions and user expectations:

- **Compressor**: Threshold, ratio, attack, release, makeup gain
- **EQ**: Frequency bands, Q/resonance, gain controls
- **Reverb**: Room size, decay, damping, mix
- **Synth**: Oscillators, filters, envelopes, modulation
- **Utility**: Gain, pan, phase, metering

### Parameters

From creative-brief.md or existing code:

- **Names**: "Threshold", "Attack Time", "Mix"
- **Types**: Float, Bool, Choice
- **Ranges**: Min/max values with units
- **Defaults**: Initial values
- **Groupings**: Related parameters (input section, tone section)

### UI Vision

User's aesthetic preferences:

- **Layout**: "Controls on left, visualizer on right"
- **References**: "Like FabFilter Pro-C", "Vintage LA-2A style"
- **Mood**: "Minimal and clean", "Skeuomorphic analog"

### Colors

Brand identity and theme:

- **Primary**: Main accent color (#4a9eff)
- **Background**: Dark (#2b2b2b) or light (#f5f5f5)
- **Text**: High contrast for readability
- **Accent**: Warnings, clip indicators (#ff6b6b)

### Special Elements

Advanced UI features:

- **VU Meter**: Input/output levels, gain reduction
- **Frequency Analyzer**: Real-time FFT
- **Waveform Display**: Input signal visualization
- **Envelope Follower**: ADSR visual feedback

## Example Extraction

**From creative-brief.md:**

```markdown
# Vintage Compressor
- Warm, analog-style dynamics processor
- 5 parameters: threshold, ratio, attack, release, makeup gain
- UI vision: "Think LA-2A meets modern UI"
- Colors: Warm bronze (#d4a574), deep black (#1a1a1a)
- Special: VU meter with smooth ballistics
```

**Extracted context:**

- Plugin type: Compressor (dynamics processor)
- Parameters: 5 continuous controls
- UI style: Vintage-inspired but modernized
- Colors: Custom (bronze/black palette)
- Special: VU meter required
- Layout: Horizontal (analog gear convention)

## Using Context

**Guide design decisions:**

- Compressor → Horizontal layout (signal flow left-to-right)
- Vintage style → Rotary knobs (not sliders)
- Custom colors → Use bronze/black theme (not default dark)
- VU meter → Allocate space in header or footer
- 5 parameters → Simple grid works (not complex multi-section)

**Fill gaps with questions:**

If creative brief says "vintage compressor" but doesn't specify:
- Window size → Ask (or default to 600x400)
- Control positions → Ask (or use standard horizontal flow)
- Font → Use classic sans-serif (Inter, no script fonts for readability)

## Example: Extracting from creative-brief.md

**Creative brief excerpt:**
```
# LushVerb - Creative Brief

## Core Concept
A lush, musical reverb with three intuitive controls: Size, Damping, Mix.

## Visual Aesthetic
- Dark, moody background (deep blue-grey)
- Three large rotary knobs in horizontal row
- Vintage hardware-inspired styling
```

**Extracted context for Phase 1.5:**
```
Plugin: LushVerb
Concept: Lush, musical reverb
Controls: Size, Damping, Mix (three rotary knobs)
Layout: Horizontal row
Style: Vintage hardware, dark moody (deep blue-grey background)
```

**What to extract:**
- Plugin name
- Core concept (one sentence)
- Control list (names and types)
- Layout preference (if specified)
- Visual aesthetic keywords (colors, style, mood)

**What to skip:**
- Detailed DSP descriptions
- Technical implementation notes
- Unrelated sections (if creative brief has multiple plugins)
