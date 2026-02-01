# O-Detune Creative Brief

**Plugin Name:** O-Detune
**Type:** Audio Effect (Detuning / Pitch Thickening)
**Created:** 2026-02-01
**Status:** Ideated

---

## Vision Statement

A colorful lo-fi detuning plugin that combines analog tape wobble with unison thickness in one mono-safe package. Fills the gap between MicroShift's clinical widening and RC-20's tape character.

**Tagline:** "Tape wobble meets unison thickness - colorful detuning, mono-safe."

---

## Market Position

### The Gap We Fill

No existing plugin combines:
1. **Tape-style wobble** (wow/flutter pitch modulation)
2. **Unison detuning** (multi-voice supersaw-style thickening)
3. **Mono compatibility** (guaranteed phase-safe summing)
4. **Colorful character** (saturation and personality)

Users currently chain MicroShift + RC-20 for this combination. O-Detune does it in one plugin.

### Competitive Analysis

| Competitor | Strengths | What We Do Better |
|------------|-----------|-------------------|
| MicroShift ($99) | Industry standard widening | Add tape character + mono-safe |
| Wow Control ($129) | Best tape simulation | Add unison mode + lower price |
| RC-20 ($59) | All-in-one lo-fi | Focused tool, simpler workflow |
| Wider (FREE) | Perfect mono compatibility | Add pitch modulation + character |

### Target Price: $49-69

---

## Core Concept

### Dual-Mode Engine

**Mode A: Wobble**
- Tape-style wow/flutter pitch modulation
- Rate: 0.1 - 10 Hz (covers slow wow to fast flutter)
- Depth: 0 - 100 cents
- Shape: Sine / Triangle / Random (non-repeating)
- Era presets: 60s (Ampex), 70s (Teac), 80s (Cassette)

**Mode B: Unison**
- Multi-voice detuning (supersaw-style for any audio)
- Voices: 2, 3, 4, 5, 7
- Detune: 0 - 50 cents total spread
- Distribution: Linear / Exponential / Random
- Stereo spread per voice

**Blend Control**
- Crossfade between Wobble and Unison (0-100%)
- Enables hybrid effects (wobbling unison voices)

### Character Section

| Parameter | Range | Purpose |
|-----------|-------|---------|
| Drive | 0-100% | Subtle warmth to tube saturation |
| Color | Dark ↔ Bright | Tone shaping / filtering |
| Age | 0-100% | Combined degradation (hiss + filtering + drift) |

### Essential Controls

| Parameter | Range | Purpose |
|-----------|-------|---------|
| Width | Mono ↔ Extra Wide | Stereo spread of effect |
| Mix | 0-100% | Wet/dry blend |
| Focus | 20Hz - 20kHz | Frequency-selective processing |
| Mono-Safe | Toggle | Guarantees mono compatibility |

### Advanced (Expandable Panel)

- Delay: 0-50ms pre-delay for spatial depth
- Feedback: Recirculate for intensifying effect
- Tempo Sync: Lock wobble rate to DAW BPM
- Randomization: Per-voice variation amount

---

## Primary Use Cases

| Use Case | Mode | Typical Settings |
|----------|------|------------------|
| **Vocal thickening** | Unison | 3 voices, 10 cents, moderate Drive |
| **Synth widening** | Unison | 5-7 voices, 25 cents, low Drive |
| **Pad movement** | Wobble | Low rate, medium depth, subtle Age |
| **Lo-fi vocals** | Blend 50% | 70s era, Age up, both modes active |
| **Creative destruction** | Wobble | High rate/depth, Feedback, extreme Age |
| **Bass fattening** | Unison | 2 voices, 5 cents, Focus on low-mids |
| **Instrument doubling** | Unison | 3 voices, 8 cents, wide stereo |

---

## Technical Requirements

### DSP Architecture

**Wobble Engine:**
- Delay-based pitch modulation via variable delay lines
- Multiple LFO sources with noise modulation for authentic non-repeating patterns
- Era-specific frequency response curves
- Triangle/sine/random LFO shapes

**Unison Engine:**
- N parallel pitch shifters (delay-based for low latency)
- Configurable voice distribution across detune range
- Per-voice stereo panning
- Central voice at unity pitch as tonal anchor

**Mono-Safe Mode:**
- All-pass/comb filtering approach (Wider-style)
- Effect cancels cleanly on mono sum
- Visual indicator of mono compatibility status

**Character Processing:**
- Analog-modeled saturation (tube-style harmonics)
- Tone filtering (high-shelf / low-pass blend)
- Subtle noise injection (Age parameter)

### Performance Targets

| Metric | Target |
|--------|--------|
| Latency | < 256 samples (5.8ms @ 44.1kHz) |
| CPU | < 3% on Apple Silicon |
| Memory | < 50MB |
| Sample rates | 44.1kHz - 192kHz |

### Format Support

- VST3, AU, AAX (64-bit)
- macOS 11+ (Apple Silicon native)
- Windows 10+ (future consideration)

---

## UI/UX Vision

### Aesthetic Direction

**Style:** Colorful lo-fi - vintage synth meets cassette culture
- Warm color palette: burnt orange, teal, cream, muted purple
- Not skeuomorphic hardware - modern but characterful
- Animated visualization of pitch wobble/voice spread
- Distinctive, inspiring to look at

### Layout Concept

```
┌─────────────────────────────────────────┐
│  O-DETUNE                    [Presets]  │
├─────────────────────────────────────────┤
│                                         │
│    ┌─────────┐         ┌─────────┐     │
│    │ WOBBLE  │ ←BLEND→ │ UNISON  │     │
│    │  [Era]  │   [●]   │ [Voices]│     │
│    │  Rate   │         │ Detune  │     │
│    │  Depth  │         │ Spread  │     │
│    └─────────┘         └─────────┘     │
│                                         │
│    ┌─────────────────────────────┐     │
│    │  DRIVE    COLOR    AGE      │     │
│    │   [●]      [●]     [●]      │     │
│    └─────────────────────────────┘     │
│                                         │
│    WIDTH  ━━━━━━━●━━━━━━━  FOCUS       │
│                                         │
│    [MONO-SAFE]          MIX [●]        │
│                                         │
│    ▼ Advanced                           │
└─────────────────────────────────────────┘
```

### Key Interactions

- Mode selector with visual feedback (wobble animation vs voice visualization)
- Mono-Safe indicator (green = safe, yellow = potential issues)
- Single "Vibe" macro knob for instant gratification (optional)
- A/B comparison

---

## Preset Categories

1. **Thickening** - Subtle enhancement presets
2. **Widening** - Stereo spread focused
3. **Lo-Fi** - Tape wobble + degradation
4. **Movement** - Evolving modulation
5. **Destruction** - Extreme creative effects
6. **Vocals** - Optimized for voice
7. **Instruments** - Guitar, keys, etc.

---

## Success Criteria

1. **Fills the gap** - Users stop chaining MicroShift + RC-20
2. **Mono-safe works** - Effect translates to all playback systems
3. **Sounds colorful** - Not sterile like clinical wideners
4. **Simple but deep** - Great with one knob, powerful with all
5. **CPU efficient** - Usable on multiple tracks
6. **Inspiring UI** - Users want it open

---

## Research References

- Full market research: `research/O-Detune-market-research.md`
- Key competitors: Soundtoys MicroShift, Goodhertz Wow Control, XLN RC-20, Polyverse Wider
- Technical references: H3000 algorithms, tape wow/flutter modeling, granular pitch shifting

---

## Next Steps

1. **Stage 0:** Architecture planning - DSP specification, complexity assessment
2. **UI Mockup:** Design visual system before implementation
3. **Stage 1-4:** Implementation workflow

---

*Brief created: 2026-02-01*
