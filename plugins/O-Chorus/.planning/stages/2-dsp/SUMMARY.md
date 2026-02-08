# Stage 2: DSP — Execution Summary

**Plugin:** O-Chorus
**Stage:** 2-dsp
**Completed:** 2026-02-07
**Agent:** dsp-agent (manual execution)

---

## What Was Built

### ChorusEngine (`Source/DSP/ChorusEngine.h`, `Source/DSP/ChorusEngine.cpp`)

Complete multi-voice BBD-style chorus DSP engine implementing:

1. **Multi-Voice Delay Line Engine** — Array of 8 `DelayLine<float, Lagrange3rd>` voices (first N active based on voices parameter). 50ms max delay buffer. Seeded per-voice depth variation (0.85-1.15 multiplier) for organic feel.

2. **LFO Modulation** — Sine wave oscillator with per-voice phase offset (`2pi * i / numVoices`). Phase accumulator with wrap at 2pi. Rate 0.05-5 Hz, depth controls ±5ms modulation around 10ms base delay.

3. **Tanh Saturation** — Asymmetric soft-clipping (positive half 1.0x, negative 0.9x) for BBD character. Normalized to maintain unity gain. Drive scaled to 0-0.5 range for subtle warmth. Bypassed when drive < 0.01.

4. **One-Pole Tone Filter** — `juce::dsp::IIR::Filter<float>` stereo pair. Piecewise linear mapping: tone -1 to +1 maps to 2kHz-8kHz-20kHz cutoff. Applied to summed wet signal.

5. **Equal-Power Stereo Panning** — Per-voice pan positions distributed evenly across stereo field. Width parameter scales effective pan from mono (0) to full spread (1). Constant-power law (cos/sin).

6. **Voice Count Crossfade** — 50ms crossfade when voice count changes. Blends old and new voice counts to prevent clicks. Recalculates phase offsets and pan positions after crossfade.

7. **SmoothedValue Parameters** — All continuous parameters smoothed (50ms ramp, 100ms for tone) to prevent zipper noise. Voices parameter (discrete int) handled via crossfade instead.

### Signal Flow
```
Stereo In → Mono Sum → N Voices (LFO→Delay→Saturate→Pan) → Sum Wet L/R → Tone Filter → Mix with Dry → Stereo Out
```

### Processor Integration
- `PluginProcessor.h` — Added `ChorusEngine` member
- `PluginProcessor.cpp` — Wire `prepare()` in `prepareToPlay()`, read all 7 params via atomic loads in `processBlock()`, call `chorusEngine.process()`

### CMakeLists.txt
- Added `Source/DSP/ChorusEngine.cpp` to `target_sources`

---

## Build Results

- VST3: Compiled successfully (0 errors, 0 warnings)
- AU: Compiled successfully
- AU Detection: `aufx OuCh OuDv - Ouaricon Audio Development: O-Chorus-dev`
- Installed to system plugin folders

---

## All 7 Parameters Connected

| Parameter | ID | Range | DSP Component |
|-----------|----|-------|---------------|
| Rate | `rate` | 0.05-5.0 Hz | LFO phase increment |
| Depth | `depth` | 0.0-1.0 | Delay modulation amount |
| Voices | `voices` | 1-8 | Active voice count |
| Width | `width` | 0.0-1.0 | Stereo pan spread |
| Tone | `tone` | -1.0 to +1.0 | IIR lowpass cutoff |
| Mix | `mix` | 0.0-1.0 | Dry/wet crossfade |
| Drive | `drive` | 0.0-1.0 | Tanh saturation amount |

---

## Files Created

- `Source/DSP/ChorusEngine.h` (new)
- `Source/DSP/ChorusEngine.cpp` (new)

## Files Modified

- `Source/PluginProcessor.h` (added ChorusEngine include + member)
- `Source/PluginProcessor.cpp` (wired DSP into prepare/process)
- `CMakeLists.txt` (added DSP source file)
