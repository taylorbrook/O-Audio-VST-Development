# Phase 3.4 Summary: Impossible Physics + Dual Bore

**Completed:** 2026-04-05
**Stage:** 2 (DSP)
**Phase:** 3.4

---

## What Was Implemented

### BoreWaveguide.h — 3 new features in updateParams()
- **Infinite sustain:** Viscothermal gain ramps 0.995→1.0, bell cutoff pushes toward Nyquist (total reflection). At default 0: identical to Phase 3.3
- **Reverse bore:** Segment center positions interpolate from normal to reversed (hichiriki-like inverted taper). Only audible with boreCharacter > 0
- **Bore profile:** Per-segment taper ratios interpolate Simple(1:1:1:1:1) → Multi(0.3:0.5:1.0:1.2:2.0). Effective halfAngle clamped to 5° max with near-zero guard per segment

### ReedWindVoice — bore2 (dual bore) + feedback cross-coupling
- Added `BoreWaveguide bore2` + `prevBore2Minus` members
- Wired prepare/reset/legato for bore2 alongside bore1
- Per-block: 6 new parameter reads (infiniteSustain, reverseBore, dualBore, dronePitchCents, feedbackPath, boreProfile)
- Dual bore: bore2 gets same params + tone holes, frequency offset by dronePitchCents/1200
- Feedback cross-coupling: `safeFeedback = feedbackPath * 0.5f` caps at 50% for stability
- Output: mixed radiated output from both bores when dual active
- Voice cleanup: checks both bore energies before clearing note

### PluginProcessor.cpp — dronePitch parameter migration
- Changed from semitones (-24..24, v1) to cents (-2400..2400, v2)
- Added v1→v2 migration in setStateInformation()

## Files Modified

| File | Changes |
|------|---------|
| `Source/DSP/BoreWaveguide.h` | updateParams extended with 3 params, infinite sustain + reverse bore + bore profile |
| `Source/ReedWindVoice.h` | bore2 member, prevBore2Minus member |
| `Source/ReedWindVoice.cpp` | bore2 lifecycle, 6 param reads, dual bore wiring, feedback cross-coupling |
| `Source/PluginProcessor.cpp` | dronePitch v1→v2 (semitones→cents), state migration |

## Validation

- VST3 + AU build: **zero errors, zero warnings**
- auval: **PASS**
- pluginval Level 5: **SUCCESS**
- Regression: all 6 new params at default → Phase 3.3 behavior preserved

## Active Parameters

30 parameters active in DSP (24 from Phase 3.3 + 6 new: infiniteSustain, reverseBore, dualBore, dronePitch, feedbackPath, boreProfile)
