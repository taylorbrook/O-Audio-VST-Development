# O-FreqPulse - Creative Brief

## One-Liner
A rhythmic gate that knows about frequency — pulse your highs at 1/16, keep your lows solid, and paint spectral patterns on a 2D grid.

## Concept

Traditional trance gates affect the whole signal uniformly. O-FreqPulse combines FFT spectral processing with step sequencing — different rhythmic patterns for different frequency regions. Paint on a frequency × time grid, generate Euclidean rhythms per band, or create evolving spectral animations.

**Core Innovation:** The intersection of spectral processing and rhythmic sequencing, presented through a hybrid interaction model that scales from simple band-based gating to complex spectral painting.

## Target Users

- **EDM Producers** — Rhythmic sidechain alternatives, frequency-aware pumping
- **Ambient Artists** — Evolving spectral textures, slow spectral morphs
- **Sound Designers** — Complex rhythmic patterns, spectral animation for SFX
- **Experimental Musicians** — Polyrhythmic spectral layering

## Key Differentiators

1. **2D Visual Grid** — Frequency × time canvas (unique in the gate/sequencer space)
2. **Euclidean Rhythm Generation** — Per-band algorithmic patterns
3. **Multiple Polyrhythmic Lanes** — Independent rhythm lengths per frequency region
4. **Spectral Freeze Steps** — Hold frequency content at specific steps
5. **Drawable Per-Step Frequency Masks** — Fine control beyond discrete bands
6. **Hybrid Interaction** — Discrete bands for quick use, paint mode for deep control

## Use Cases

### 1. Rhythmic Spectral Gating (Primary)
Classic trance gate evolved:
- Bass (sub-200Hz) stays solid or pulses slowly
- Mids (200Hz-2kHz) pump at 1/8 notes
- Highs (2kHz+) chop at 1/16 for energy

### 2. Spectral Animation Canvas
Experimental sound design:
- Paint frequency sweeps that evolve over bar cycles
- Create "spectral wipes" — frequencies revealing over time
- Animate filter-like movements without actual filtering

### 3. Polyrhythmic Spectral Layers
Complex rhythmic textures:
- 5-step Euclidean on low-mids
- 7-step Euclidean on highs
- 3-step on sub-bass
- All interlocking in polyrhythmic patterns

### 4. Spectral Freeze Effects
Granular-adjacent capabilities:
- Freeze high frequencies on beat 1, let them evolve
- Create "spectral stutters" by freezing then releasing bands

## Interaction Model

### Default Mode: Discrete Bands
- **4 frequency bands** by default: Sub (20-120Hz), Low (120-500Hz), Mid (500-4kHz), High (4kHz-20kHz)
- **Step sequencer rows** per band (8/16/32 steps)
- **Euclidean generator** per row with steps/pulses/offset
- Quick, intuitive, immediate results

### Advanced Mode: Paint Canvas
- **Full frequency resolution** on Y-axis (logarithmic scale)
- **Freehand drawing tools** — brush, line, rectangle
- **Copy/paste regions** for pattern building
- **Gradient fills** for smooth transitions

### Shared Controls
- **Global tempo sync** (1/4, 1/8, 1/16, 1/32, triplets, dotted)
- **Per-band/region gain** (not just on/off, but 0-100% depth)
- **Attack/Release** per step or global (smoothing)
- **Mix/Dry-Wet** control

## Parameter Architecture (v1.0 - Focused Foundation)

### Global Parameters
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Mix | 0-100% | 100% | Dry/wet blend |
| Steps | 4/8/16/32 | 16 | Sequence length |
| Rate | 1/1 to 1/32 | 1/16 | Step rate (tempo-synced) |
| Swing | 0-100% | 0% | Timing swing amount |
| Smoothing | 0-100ms | 5ms | Attack/release for gain changes |

### Per-Band Parameters (×4 bands)
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Band Enable | On/Off | On | Bypass this band's sequencing |
| Low Freq | 20-20kHz | varies | Band low cutoff |
| High Freq | 20-20kHz | varies | Band high cutoff |
| Depth | 0-100% | 100% | How much gain reduction on "off" steps |
| Euclidean Steps | 1-32 | 16 | Total steps in Euclidean pattern |
| Euclidean Pulses | 1-32 | 8 | Active pulses in pattern |
| Euclidean Offset | 0-31 | 0 | Rotation offset |
| Pattern Mode | Manual/Euclidean | Manual | Pattern source |

### Step Grid (per band × steps)
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Step On/Off | Boolean | Off | Step active state |
| Step Gain | 0-100% | 100% | Per-step gain (when on) |

## Technical Considerations

### FFT Processing
- **FFT Size:** 2048 samples (good frequency resolution, ~46ms latency at 44.1kHz)
- **Overlap:** 4× (75% overlap for smooth reconstruction)
- **Window:** Hann window for minimal spectral leakage
- **Reconstruction:** Overlap-add synthesis

### Latency Management
- Report latency to DAW for proper compensation
- Consider smaller FFT option (1024) for lower latency mode

### Efficiency
- Process only active bands (bypass FFT if all bands disabled)
- Efficient bin grouping for band processing
- SIMD optimization for bin multiplication

## Visual Design Direction

### Grid Visualization
- **Dark background** with subtle grid lines
- **Frequency axis (Y):** Logarithmic, labeled at octave boundaries
- **Time axis (X):** Steps with beat markers emphasized
- **Active cells:** Bright accent color (brand color)
- **Intensity:** Brightness or saturation indicates gain level

### Interaction Feedback
- **Playhead:** Vertical line showing current step
- **Hover preview:** Show affected frequency range
- **Real-time spectrum:** Optional overlay showing actual input spectrum

### Mode Switching
- Clear visual distinction between Band Mode and Paint Mode
- Smooth transition animation between modes

## Modular Expansion Roadmap

### v1.0 - Foundation (This Brief)
- 4-band discrete mode
- Euclidean generation
- Basic step sequencing
- Mix/smoothing controls

### v1.1 - Paint Mode
- Freehand drawing on full frequency grid
- Brush tools
- Region selection

### v1.2 - Advanced Sequencing
- Per-step attack/release
- Probability per step
- Pattern chaining (A/B patterns)

### v1.3 - Modulation
- LFO modulation of band parameters
- Envelope follower input
- Sidechain trigger mode

### v2.0 - Spectral Freeze
- Freeze/hold functionality per band
- Spectral blur/smear options
- Granular-style controls

## Competitive Landscape

| Product | Approach | Gap O-FreqPulse Fills |
|---------|----------|----------------------|
| Cableguys VolumeShaper | Whole-signal envelope | No frequency awareness |
| iZotope Stutter Edit | Rhythmic effects | Beat-focused, not spectral |
| Goodhertz Trem Control | Tremolo | Single-band, no sequencing |
| Sugar Bytes Effectrix | Multi-effect sequencer | Effects, not spectral gates |
| Native Instruments The Finger | Performance tool | Complex, not visual grid |

**O-FreqPulse Unique Position:** Visual 2D spectral sequencer with Euclidean generation — no direct competitor.

## Success Criteria

1. **Immediate Usability:** User can create interesting rhythmic patterns within 30 seconds using band mode
2. **Visual Clarity:** Grid clearly communicates what's happening to the sound
3. **CPU Efficiency:** <5% CPU on typical session at 44.1kHz
4. **Latency:** Acceptable for mixing (report to DAW), with low-latency option
5. **Sound Quality:** Clean spectral processing without obvious artifacts

## Open Questions for Planning Phase

1. **FFT Size Options:** Should users be able to choose FFT size (latency vs resolution tradeoff)?
2. **Band Crossover:** Hard cutoffs or smooth crossfades between bands?
3. **Step Resolution:** Fixed steps or variable per-band?
4. **Preset Strategy:** What preset categories make sense?

---

*Created: 2026-02-03*
*Status: Ideation Complete — Ready for /plan*
