# Context: Eliminate Clicks & Smooth Windowing

**Milestone:** eliminate-clicks-smooth-windowing
**Plugin:** O-Freeze
**Base Version:** 1.0.1
**Created:** 2026-02-02

---

## User Requirements

### Primary Goal
Eliminate audible clicks occurring throughout the freeze cycle and implement warmer, smoother windowing for artifact-free granular textures.

### Symptom Details

| Aspect | User Feedback |
|--------|---------------|
| **Click Location** | All phases: engage, sustained, and release |
| **Severity** | Noticeable clicks (clearly audible but not jarring) |
| **Drift Correlation** | No correlation - clicks occur regardless of Drift setting |
| **Windowing Goal** | Both edge artifact elimination AND warmer sound character |

---

## Technical Analysis

### Current Implementation

```cpp
// Grain structure
struct Grain {
    int startSample;  // Position within grain window
    int position;     // Position in freeze buffer
    bool active;
};

// 8 grains with 87.5% overlap
grainTriggerInterval = grainSize / 8;

// Asymmetric Blackman-Harris window (60% attack, 40% release)
// Attack: stretched rise from 0 to peak
// Release: compressed fall from peak to 0

// Crossfade: 50ms fade-in, 100ms fade-out
freezeGain.reset(sampleRate, 0.050);
```

### Identified Click Sources

1. **Freeze Engage Transition**
   - All 8 grains triggered simultaneously with staggered positions
   - Grains start reading from arbitrary points (not zero-crossings)
   - Buffer may contain discontinuous audio at capture moment

2. **Sustained Freeze Clicks**
   - New grains triggered every `grainTriggerInterval` samples
   - Grain start position based on current `writePosition` minus `grainSize`
   - No zero-crossing detection for grain boundaries
   - Phase cancellation between grains reading different buffer regions

3. **Freeze Release Transition**
   - Grains deactivated abruptly when freeze released
   - Linear crossfade (not perceptually-weighted)
   - Dry signal may have different phase than frozen output

4. **Window Edge Artifacts**
   - Blackman-Harris has non-zero endpoints (albeit small)
   - Asymmetric window may have subtle discontinuities at attack/release junction
   - No fade-to-zero guarantee at grain edges

---

## Requirements Specification

### R1: Click-Free Freeze Engagement
- Implement zero-crossing detection for initial grain positions
- Add soft onset ramp for first grain cycle (beyond global crossfade)
- Ensure buffer capture starts from stable audio region

### R2: Click-Free Sustained Playback
- Align grain trigger points to zero-crossings in freeze buffer
- Implement per-grain micro-fade at boundaries
- Consider phase-coherent grain overlap strategy

### R3: Click-Free Freeze Release
- Extend release crossfade with exponential decay curve
- Fade out active grains individually before global crossfade
- Ensure phase continuity with incoming dry signal

### R4: Warmer Windowing
- Evaluate window alternatives: Tukey, raised cosine, custom sigmoid
- Extend window tails for gentler onset/offset
- Reduce high-frequency content through softer attack shape

### R5: Edge Artifact Elimination
- Guarantee true zero at window endpoints
- Apply micro-fade at grain boundaries as safety net
- Test with high-frequency content to verify artifact elimination

---

## Success Criteria

1. **No audible clicks** at any point in freeze cycle (engage, sustain, release)
2. **Warmer, less harsh** frozen texture character
3. **Smooth transitions** between dry and frozen states
4. **No regression** in existing functionality (Drift, Mix, Threshold mode)
5. **Comparable CPU usage** (within 10% of current implementation)

---

## Out of Scope

- New parameters or UI changes
- Changes to grain count or overlap ratio
- Threshold mode algorithm changes
- Buffer size modifications

---

## Next Phase

Research phase will investigate:
- Zero-crossing detection algorithms for real-time audio
- Window function comparison for granular synthesis
- Phase-coherent grain overlap techniques
- Reference implementations from academic granular synthesis papers
