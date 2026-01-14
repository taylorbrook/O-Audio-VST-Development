# OuariconSimpleReverb Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.3.2
- **Type:** Audio Effect (Reverb)
- **Complexity:** 4.2 (Complex)

## Lifecycle Timeline

- **2026-01-13:** Creative brief created - lightweight reverb for instrument chains
- **2026-01-13 (Stage 0):** Research & Planning complete - Architecture and plan documented (Complexity 4.2, Phase-based implementation)
- **2026-01-13 (v1.0.0):** Initial release - All 3 stages complete, plugin installed
- **2026-01-13 (v1.0.1):** Bug fixes - Fixed knob interactivity, reverb type switching, title update
- **2026-01-13 (v1.1.0):** Type-specific DSP - Each reverb type now has distinct sonic character through dedicated processing chains (pre-delay, early reflections, all-pass dispersion, modulation, shimmer, type-specific EQ)
- **2026-01-13 (v1.2.0):** UI overhaul - Botanical seed knob design, VU meter, character display
- **2026-01-13 (v1.2.1):** VU meter fix - Connected meter to audio output, dial alignment
- **2026-01-13 (v1.3.0):** LP filter & VU improvements - Added 20-400Hz lowpass filter with toggle, VU meter now -90°/+90° with dB indicators and green-to-red color gradient, flora background 2x larger
- **2026-01-13 (v1.3.1):** Filter & Decay fixes - Changed LP filter to Low Cut (highpass), toggle is now clickable button, Decay is now 0.5x-2.0x multiplier
- **2026-01-13 (v1.3.2):** UI polish - Decay centered at 1.0x, Hz indicators on Low Cut, removed footer

## Known Issues

None

## Additional Notes

### Concept
Lightweight, CPU-efficient reverb designed to add subtle color and realism to instrument chains. Removes the "in-a-box" feel without dominating the mix.

### Parameters (8 total)
1. **Type** (Dropdown): Booth, Room, Hall, Spring, Plate, Ambient
2. **Character** (Knob): Warm ← → Bright
3. **Low Cut** (Knob): High-pass filter cutoff (20-400Hz), cuts bass from reverb
4. **Wet** (Knob): Reverb signal level
5. **Dry** (Knob): Original signal level
6. **Decay** (Knob): Tail length multiplier (0.5x-2.0x)
7. **Size** (Knob): Virtual room dimensions
8. **Low Cut On** (Toggle): ON/OFF button below Low Cut knob

### Design
- Aesthetic: Ouaricon Naturalist (botanical theme)
- Priority: CPU efficiency over complexity
- Future: Module version for embedding in other VSTs

### Sound Character
- Subtle, musical, transparent
- Natural early reflections
- Smooth decay
- Not a special effect reverb - utility focused

### DSP Architecture (v1.1.0)
**Core Components:**
- **Reverb Engine:** juce::dsp::Reverb (6 type variations with type-specific presets)
- **Pre-Delay Lines:** Stereo delay lines (3-50ms per type)
- **Early Reflections:** 4 comb filters per channel with prime-number delays
- **All-Pass Dispersion:** 3-stage all-pass chain for Spring metallic chirp
- **Modulation LFO:** Configurable rate/depth for Spring flutter and Ambient movement
- **Plate Shimmer:** Ring modulation for bright shimmering character
- **Type-Specific EQ:** High-pass, shelves, peak filters per type
- **Character Filter:** juce::dsp::IIR::Filter (warm/bright/neutral)
- **Dry/Wet Mixer:** Manual mixing (independent dry/wet gains)

**Type-Specific Processing:**
| Type | Pre-Delay | Early Ref | All-Pass | Modulation | Shimmer | EQ |
|------|-----------|-----------|----------|------------|---------|-----|
| Booth | 3ms | Minimal | - | - | - | HP 150Hz |
| Room | 15ms | Natural | - | - | - | None |
| Hall | 50ms | Spacious | - | Subtle | - | HS -2dB @3kHz |
| Spring | 20ms | Minimal | ✓ | 4.5Hz flutter | - | Peak +4dB @800Hz |
| Plate | 8ms | Dense | - | - | ✓ | HS +3dB @5kHz |
| Ambient | 35ms | Spread | - | 0.4Hz slow | - | HS -3dB @2.5kHz |

**CPU Target:** ~25-35% single core at 48kHz (moderate)

### Implementation Plan
**Strategy:** Phase-based implementation (Complex plugin, score 4.2)

**DSP Phases:**
1. Phase 3.1: Core Processing (Room reverb + dry/wet)
2. Phase 3.2: Type Switching (6 types)
3. Phase 3.3: Character Control (warm/bright filter)

**GUI Phases:**
1. Phase 4.1: Layout and Basic Controls (Ouaricon Naturalist aesthetic)
2. Phase 4.2: Parameter Binding (6 parameters)

### References
- Creative brief: `plugins/OuariconSimpleReverb/.ideas/creative-brief.md`
- DSP architecture: `plugins/OuariconSimpleReverb/.ideas/architecture.md`
- Implementation plan: `plugins/OuariconSimpleReverb/.ideas/plan.md`
- Reference plugins: FlutterVerb, DriveVerb, LushVerb

### Next Steps
Run `/implement OuariconSimpleReverb` to begin Stage 1 (Foundation + Shell)
