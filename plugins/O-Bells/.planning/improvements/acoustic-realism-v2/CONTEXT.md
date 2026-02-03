# O-Bells Acoustic Realism v2 - Requirements Context

**Milestone:** acoustic-realism-v2
**Created:** 2026-02-02
**Base Version:** 1.2.0
**Target Version:** 1.3.0 (MINOR)

---

## Executive Summary

Five DSP improvements to make O-Bells sound more realistic and organic:

1. **Shimmer Quality** - Desynchronize LFOs, wider rate range
2. **Bloom Fix** - Debug current implementation, implement spectral bloom
3. **Material Differentiation** - Replace slider with dropdown, exaggerate differences
4. **Inharmonicity Randomization** - Per-note variation for organic repeated strikes
5. **Attack Noise Overhaul** - Convolution-style impulse + volume control

---

## Improvement 1: Shimmer Quality

### Problem
Current shimmer implementation has synchronized LFO artifacts - you can hear the modulation patterns aligning, creating a mechanical/synthetic sound.

### Current Implementation
- LFO rates: 0.5-3 Hz with prime multipliers (1.0, 1.1, 1.3, 1.7...)
- LFO waveshape: All sine waves
- Depth: 1-5 cents (uniform across partials)
- Shimmer increases with decay progress

### Required Changes
- **Wider rate range**: 0.1-8 Hz (was 0.5-3 Hz)
- **Keep depth controlled by parameter** - don't vary depths per partial
- **More variation in rates** - use larger prime multipliers or randomized offsets
- **Random initial phases** - already implemented, verify working

### Expected Behavior
- As shimmer increases, partials drift in frequency at different rates
- No audible synchronization or pattern
- Creates organic metallic shimmer like real bells

---

## Improvement 2: Bloom Fix + Spectral Bloom

### Problem
Current bloom parameter doesn't produce audible effect. User suspects implementation bug.

### Current Implementation Analysis
```cpp
// initializeBloom: sets amplitude to 30-70% of target
float initialFraction = juce::jmap(currentBloom, 0.7f, 0.3f);
partial.amplitude = partial.initialAmplitude;

// applyBloom: cosine interpolation back to peak
partial.amplitude = partial.initialAmplitude + (peak - initial) * cosineT;
```

**Potential Bug:** Bloom may be fighting with multi-stage decay coefficients. Need to verify bloom completes before decay takes over.

### Required Changes

**Part A: Debug Amplitude Bloom**
- Trace bloom execution path
- Ensure bloom phase completes before decay dominates
- Consider longer bloom duration range (up to 200ms)

**Part B: Implement Spectral Bloom (Option 3)**
- Low partials (0-2): Start at full amplitude, no bloom delay
- Mid partials (3-4): Start at 50%, bloom over 50ms
- High partials (5-7): Start at 10-20%, bloom over 80-150ms
- Creates "opening up" spectral movement on attack

### Expected Behavior
- At bloom=0%: No bloom, instant full spectrum
- At bloom=50%: Subtle high-partial fade-in
- At bloom=100%: Dramatic "woooosh" as high partials swell in

---

## Improvement 3: Material Differentiation

### Problem
All 5 materials sound nearly identical. Current differences are too subtle:
- Decay: 0.7x - 1.4x (barely audible)
- Brightness: ±0.15 (subtle)
- Inharmonicity: ±0.05 (imperceptible)

### Current Materials
| Material | Decay | Brightness | Inharm |
|----------|-------|------------|--------|
| Bronze | 1.0x | 0.0 | 0.0 |
| Brass | 0.9x | +0.05 | +0.02 |
| Steel | 1.4x | +0.10 | +0.01 |
| Aluminum | 0.7x | +0.15 | +0.05 |
| Cast Iron | 1.2x | -0.10 | +0.03 |

### Required Changes

**Part A: UI Change**
- Replace continuous slider with discrete dropdown
- 5 options: Bronze, Brass, Steel, Aluminum, Cast Iron

**Part B: Exaggerate Material Properties**
Research-based but amplified for audible difference:

| Material | Decay | Brightness | Inharm | Special Character |
|----------|-------|------------|--------|-------------------|
| Bronze | 1.0x | 0.0 | 0.0 | Baseline warm church bell |
| Brass | 0.7x | +0.20 | +0.08 | Bright, shorter, jazzy |
| Steel | 2.0x | +0.25 | -0.05 | Very bright, long sustain |
| Aluminum | 0.5x | +0.30 | +0.12 | Very bright, short, thin |
| Cast Iron | 1.5x | -0.25 | +0.15 | Dark, long, gamelan-like |

**Part C: Additional Material Effects**
- Different attack character per material (affects strike noise)
- Different decay envelope shapes (not just time)

### Expected Behavior
- Each material should be immediately distinguishable
- Material selection should dramatically change the bell character
- Dropdown provides clear categorical choices

---

## Improvement 4: Inharmonicity Randomization

### Problem
Repeated strikes of the same note sound identical - unrealistic for physical bells where strike position/force varies.

### Current Implementation
- Partial ratios interpolated deterministically from 3 tables
- Same ratios every time for same inharmonicity setting

### Required Changes

**Per-Note Randomization (on each startNote):**
- **Pitch variation**: ±5-15 cents random offset per partial
- **Amplitude variation**: ±20-30% random scaling per partial
- Use Gaussian distribution centered on nominal values

**Implementation Approach:**
```cpp
// In initializePartials():
float pitchOffset = gaussianRandom() * 10.0f;  // ±10 cents std dev
float ampVariation = 1.0f + gaussianRandom() * 0.25f;  // ±25%

partial.frequency *= std::pow(2.0f, pitchOffset / 1200.0f);
partial.amplitude *= ampVariation;
```

### Expected Behavior
- Each strike sounds like different zone of same bell
- Organic variation without losing bell identity
- Multiple notes in sequence sound "alive"

---

## Improvement 5: Attack Noise Overhaul

### Problem
Current attack transients (Click/Thud/Ping) don't sound realistic - just filtered white noise.

### Current Implementation
- Click: High-pass filtered noise, 3-8ms
- Thud: Low-pass filtered noise, 15-30ms
- Ping: Bandpass resonant noise at fundamental

### Required Changes

**Part A: Convolution-Style Impulse**
Replace noise-based approach with impulse excitation through resonant filters:
1. Generate short impulse (1-2 samples)
2. Route through bank of parallel resonant filters
3. Filters tuned to first 3-4 partials with high Q
4. Mix: mallet hardness controls which filters dominate

**Part B: Attack Noise Volume Slider**
- Add new parameter: "Attack" (0-100%)
- 0% = minimal transient, pure tone
- 50% = natural transient level
- 100% = exaggerated transient, percussive

**Part C: Keep Strike Character Options**
Retain Click/Thud/Ping but apply to new impulse approach:
- Click: High-Q filters, short decay, bright
- Thud: Low-Q filters, longer decay, dark
- Ping: Medium-Q, resonant, metallic

### Expected Behavior
- Attack sounds like actual mallet hitting metal
- Natural "dong" or "clunk" character
- Volume slider provides creative control

---

## UI Changes Summary

| Change | Current | New |
|--------|---------|-----|
| Material | Slider (0-100%) | Dropdown (5 options) |
| Attack Noise | - | New slider (0-100%) |

**Parameter Count:** 21 → 22 (add attackLevel)

---

## Technical Constraints

- Real-time safe (no allocations in processBlock)
- Per-partial state must fit in existing structures
- Randomization must use deterministic seed or be per-voice
- Material dropdown requires Choice parameter type

---

## Success Criteria

1. **Shimmer**: No audible LFO synchronization at any setting
2. **Bloom**: Clearly audible spectral "opening" at bloom > 50%
3. **Materials**: Blind test can identify each material by ear
4. **Inharmonicity**: 10 repeated strikes sound subtly different
5. **Attack**: Transients sound like physical mallet impact

---

## Version Bump Justification

**MINOR version (1.2.0 → 1.3.0)**
- New parameter added (attackLevel)
- Material UI changed from slider to dropdown (breaking preset change)
- Significant DSP behavior changes
