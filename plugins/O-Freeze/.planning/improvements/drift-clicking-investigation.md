# O-Freeze Drift Clicking Investigation

**Date:** 2026-02-03
**Status:** UNRESOLVED - needs fresh investigation
**Version:** 1.2.1 (changes not yet committed)

## Original Issue
- Clicking caused by the "drift" parameter
- Harshness of clicking increases with drift amount
- User also requested reducing grain count from 12 to 8

## Investigation History

### Initial Hypothesis (INCORRECT)
Per-grain random position offsets causing phase discontinuities between overlapping grains.

**Attempted fix:** Smoothed drift - all grains share a slowly-wandering offset instead of random per-grain offsets.

**Result:** Made clicking more present but "evenly spaced"

### Second Hypothesis
COLA (Constant Overlap-Add) not working correctly - Hann windows at 87.5% overlap don't sum to constant value, causing amplitude modulation at grain trigger rate.

**Attempted fix:** Normalize each sample by actual window sum instead of assuming COLA sum ≈ 1.0

**Result:** Better but not smooth enough

### Third Attempt
Custom trapezoidal window with longer, smoother fades:
- 400ms total grain
- 175ms fade-in (half-Hann)
- 50ms sustain at -3dB peak
- 175ms fade-out (half-Hann)

**Result:** Same problem persists. User noted "grains don't sound very long" - suggesting the grain size change may not be taking effect, or there's a fundamental issue elsewhere.

## Key Observations
1. Clicking is "evenly spaced" - suggests grain trigger rate issue
2. Grains don't sound long despite 400ms setting - possible the grainSize isn't being applied correctly, or something else is truncating/affecting grain playback
3. Problem persists across multiple window shapes

## Possible Root Causes to Investigate
1. **grainSize not being used correctly** - verify the grain lifecycle actually uses the full grainSize
2. **Grain deactivation happening too early** - check `if (grain.startSample >= grainSize)` logic
3. **Window indexing issue** - verify hannWindow is indexed correctly throughout grain lifetime
4. **Buffer boundary issues** - discontinuities when grains wrap around freeze buffer
5. **The drift smoothing might still be too fast** - or drift implementation is fundamentally flawed
6. **prepareToPlay not being called** - sample rate dependent values might be stale

## Current Code State
- NUM_GRAINS = 8
- grainSize = sampleRate * 0.400 (400ms)
- Custom trapezoidal window (175ms fades, -3dB peak)
- Smoothed drift (0.000005 coefficient, 3 second target updates)
- Window sum normalization enabled

## Files Modified (uncommitted)
- Source/PluginProcessor.h
- Source/PluginProcessor.cpp
- CHANGELOG.md

## Backup Available
`backups/O-Freeze/v1.2.0/` contains pre-modification state

## Recommendation for Next Session
Start by verifying the grain lifecycle is actually using the configured grain size. Add debug logging or test with extreme values (e.g., 2 second grains) to confirm the grain duration is actually changing.
