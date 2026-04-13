# O-Reed: High Pitch + Clipping Bug — Investigation Brief

## Symptom
Plugin sounds very high-pitched rather than the correct MIDI note, and clips. Issue persists across v1.0.6–v1.0.10.

## What Was Tried (v1.0.10 session)

### Diagnostic Tests Performed
1. **Auto-test MIDI injection** — triggered MIDI note 60 (C4) programmatically, measured output
2. **Zero-crossing analysis** — confirmed zero crossings = 0 (no oscillation at all with default params)
3. **Bore impulse test** — injected 1000 Pa impulse, measured 87% energy loss per round trip
4. **Bore-only test (reed bypassed)** — confirmed bore waveguide alone disperses but doesn't destroy energy
5. **Raw circular buffer test** — bypassed bore entirely, proved feedback loop concept works (oscillation grows correctly)
6. **1-segment bore test** — single forward+backward delay with bell/visc: oscillates perfectly
7. **2-segment bore test** — also oscillates perfectly
8. **5-segment bore (original)** — 87% loss per round trip, cannot self-oscillate

### Root Cause Found (Partial)
- **toneHoleCutoff default was 1500 Hz** (not 8000 Hz), opening 3/4 tone holes with scatter=-0.165 each
- This radiated ~70% of bore energy per round trip at the tone hole junctions
- **Fixed in v1.0.10**: changed default to 8000 Hz

### What's Still Wrong (v1.0.10 still doesn't work)
The tone hole fix alone is insufficient. Even with all tone holes closed (scatter=0), the 5-segment bore topology loses far more energy than the 1-segment equivalent. The 1-segment and 2-segment versions oscillate correctly with the same filters and same total delay.

## Key Findings

### The 5-Segment Bore Has Excess Energy Loss
- **1 segment**: round-trip gain with 1.10x reed = 1.0945 per trip (grows exponentially) ✓
- **2 segments**: identical growth rate ✓
- **5 segments**: round-trip gain = ~0.13 (87% loss) even with all scatters = 0 ✗

### Verified NOT the Cause
- JUCE Thiran delay lines (work correctly in 1-seg and 2-seg tests)
- Bell allpass filter (unity gain, correct phase)
- Viscothermal filter (0.7% loss — correct)
- Conical scale factors (all 1.0 for cylindrical bore)
- `setFrequency()` being called every block (disabled — same result)
- `feedbackGain` (confirmed 1.0)
- Reed model linearized gain (computed 1.016–1.145 depending on Z_c)

### Likely Cause of Remaining 5-Segment Loss
The 5-segment version uses **very short delay lines** (halfDelay = [8.5, 17, 17, 21, 21] samples) compared to the working 2-segment version (halfDelay = 42.4 each). Something about short Thiran delays in the specific 5-segment pop-all/push-all topology is causing energy leakage. Possibilities:
1. **Thiran allpass with short fractional delays** — segment 0 has delay 8.475 (fraction=0.475). At this value, Thiran allpass may have numerical issues or the push-immediately-after-pop pattern causes the allpass to see corrupted state.
2. **Simultaneous pop of 10 delay lines** — some interaction between multiple short delays being read in the same sample could cause unexpected behavior in JUCE's DelayLine implementation.
3. **The fractions [0.10, 0.20, 0.20, 0.25, 0.25]** — the 10% segment has only 8.5 samples of delay. If JUCE's Thiran delay doesn't handle delays < 10 well, this segment would introduce loss.

## Recommended Next Steps

1. **Test 5 segments with EQUAL fractions** [0.20, 0.20, 0.20, 0.20, 0.20] — each segment gets ~17 samples. If this works, the problem is the short 8.5-sample segment.
2. **Test 5 segments with LONGER minimum** — increase first segment from 0.10 to 0.15 or 0.20.
3. **Test with Linear interpolation instead of Thiran** — if Linear works but Thiran doesn't at short delays, the Thiran implementation is the issue.
4. **If all 5-segment variants fail**: collapse to 2 segments (which works) and accept the simpler topology.

## Additional Issue: Group Delay vs Phase Delay
The bell allpass group delay (~8.9 samples at 262Hz) is used for delay compensation, but the actual PHASE DELAY at 262Hz is only ~0.12 samples. This means the bore delay is set ~8.8 samples too short, shifting pitch up by 5-17% depending on note (worse at higher pitches). This causes the "high pitched" symptom once oscillation is achieved. Fix: replace group delay compensation with phase delay for the bell allpass.

## Files Involved
- `plugins/O-Reed/Source/DSP/BoreWaveguide.h` — bore waveguide (processSample, setFrequency)
- `plugins/O-Reed/Source/ReedWindVoice.cpp` — voice DSP loop
- `plugins/O-Reed/Source/PluginProcessor.cpp` — parameter defaults, presets
- `plugins/O-Reed/Source/DSP/ReedModel.h` — reed physics model
