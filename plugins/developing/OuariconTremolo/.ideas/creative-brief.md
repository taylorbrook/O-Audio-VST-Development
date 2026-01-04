# Ouaricon Tremolo - Creative Brief

## Vision

A tremolo effect plugin that blends vintage botanical aesthetics with modern precision. Inspired by early 20th-century herbarium illustrations and scientific manuscripts, the interface evokes the organic, hand-crafted quality of botanical drawings while providing clean, musical tremolo modulation.

## Sonic Goals

**Core Effect**: Classic amplitude modulation tremolo with extended waveform options beyond the traditional sine wave.

**Character**:
- Clean, transparent modulation that enhances rather than dominates
- Multiple waveform shapes for creative sound design (sine, triangle, phasor, square, pulse, noise)
- Smoothing control to soften transitions between waveform samples
- Tempo sync capability for rhythmic integration
- Pan sync option for stereo width modulation

**Use Cases**:
- Subtle rhythmic movement on guitars, keys, pads
- Aggressive stutter/gating effects with square/pulse waveforms
- Textural ambience with noise modulation
- Tempo-locked rhythmic pulsing
- Stereo field animation

## UX Principles

**Aesthetic**: Botanical Scientific
- Vintage paper texture background (aged, warm cream tones)
- Botanical illustration overlay (carrot drawing, semi-transparent)
- Botanical unicode motifs as decorative elements (❦ fleuron, ✿ floral, ❧ leaf)
- Earthy botanical green color palette for interactive elements
- Baskerville typeface (1757, authentic 18th-century botanical publication typography)
- Hand-drawn quality meets precision interface design

**Layout Philosophy**:
- Left panel: Essential controls (sync buttons, speed/depth knobs)
- Right panel: Creative controls (waveform selection, visualizer, smoothing)
- Clear visual hierarchy - immediate access to core parameters
- Real-time waveform visualization for instant feedback

**Interaction Model**:
- Vertical drag knobs (traditional plugin ergonomics)
- Toggle buttons with botanical motif accents
- Dropdown selector for waveform types
- Horizontal slider for smoothing (continuous parameter)
- Real-time visual feedback on waveform display
- Hover states that lift elements subtly (shadow/scale)

**Visual Feedback**:
- Knobs with radial gradient, center floral symbol, ring decorations
- Active state changes for toggle buttons (color intensity, border)
- Live waveform canvas updates on any parameter change
- Grid lines in visualizer for reference
- Value displays showing exact numeric values with units

## Technical Requirements

**Plugin Formats**: VST3, AU, Standalone
**UI Technology**: WebView (HTML/CSS/JS)
**Sample Rate Support**: 44.1kHz - 192kHz
**Processing**: Real-time, low-latency amplitude modulation
**Preset Management**: DAW-standard preset system

## Design Constraints

- Fixed window size: 600×400px (maintains design proportions)
- High-DPI compatible (resolution-independent rendering)
- Readable on both light and dark DAW backgrounds
- Minimal CPU overhead for waveform rendering
- All controls must be automatable
- Parameters must respond to MIDI CC and host automation

## Success Criteria

A tremolo plugin that feels like a carefully preserved botanical specimen - beautiful to look at, precise in function, and musically useful. The interface should inspire creativity while remaining highly functional. Users should feel they're interacting with a thoughtfully crafted instrument, not just another utility plugin.
