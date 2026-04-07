# O-Formant Safety Fix: Output Limiter + Q Clamping

## Problem

O-Formant produces dangerously loud output (~148 dB above 0dBFS worst case) due to unbounded formant filter Q values and zero output protection. The symptom is an "insanely loud beep" — a resonant formant filter ringing at extreme gain. Most likely triggered by high formant shift + spread with auto-consonant enabled.

## Root Cause Chain

1. **FormantFilterBank Q has no ceiling** (`FormantFilterBank.h:65-66`): `Q = finalFreq / bw` with only a floor of 0.5, no max. Vowel bandwidths from VowelData range 40-130Hz. With formant shift +24st (4x freq multiplier) and spread=2.0, frequencies reach the Nyquist clamp at 21950Hz. Q = 21950/40 = **549**. Peak gain at resonance = ~75,000x.

2. **Bandwidth doesn't scale with formant shift** (`FormantFilterBank.h:42-68`): Shift multiplies frequency via `shiftFactor = pow(2, shift/12)` but bandwidth `bw[i]` stays fixed at the vowel's original value. In real vocal acoustics, bandwidth scales roughly proportionally with frequency. This is the primary cause of extreme Q at high shift values.

3. **No per-voice amplitude clamping** (`FormantVoice.cpp:276-280`): Only guards NaN/Inf. A sample at +100 dB passes through as long as it's finite.

4. **No output limiter** (`PluginProcessor.cpp:369-386`): processBlock applies smoothed gain then sends directly to DAW. Zero safety limiting.

5. **16 voices sum unchecked**: All voice outputs accumulate additively into the buffer with no bus limiting.

6. **Consonant burst excites all resonance peaks**: Auto-consonant plosive burst pushes broadband noise through the resonant formant filters, exciting every resonance peak simultaneously. Burst amplitude multiplied by `(2.0 - manner)` = 2x for plosives.

## Fix: 4 Layers of Safety

### Layer A: Q Clamping + Bandwidth Scaling (FormantFilterBank.h)

In `updateCoefficients()`, after computing `finalFreq` and before computing Q (~line 65):

1. **Scale bandwidth with shift factor**: `float scaledBW = bw[i] * shiftFactor;` (shiftFactor is already computed at line 42 as `pow(2, shift/12)`). This must be passed into the loop — currently shiftFactor is computed before the loop so it's available, just needs to be used.

2. **Clamp Q to max 25**: After computing Q, add `Q = std::min(Q, 25.0f);`

The updated code in the loop should be:
```cpp
// Scale bandwidth proportionally with shift to maintain consistent Q
float scaledBW = bw[i] * shiftFactor;

// Q = freq / bandwidth, clamped to safe range [0.5, 25]
float Q = finalFreq / std::max(scaledBW, 1.0f);
Q = juce::jlimit(0.5f, 25.0f, Q);
```

This replaces the existing two lines:
```cpp
float Q = finalFreq / std::max (bw[i], 1.0f);
Q = std::max (Q, 0.5f);
```

### Layer B: Per-Voice Soft Clip (FormantVoice.cpp)

In `renderNextBlock()`, after the formant filter output and ADSR envelope (line 272: `float sample = mixed * env;`), replace the existing NaN/Inf guard block (lines 275-280) with a combined soft clip + NaN guard:

```cpp
// Soft-clip to prevent extreme amplitudes from resonant filters
// std::tanh naturally limits to [-1, 1] with smooth saturation
sample = std::tanh(sample);

// NaN/Inf guard (belt-and-suspenders after tanh)
if (! std::isfinite(sample))
{
    sample = 0.0f;
    filterBank.reset();
    consonantEngine.reset();
}
```

This replaces the existing block:
```cpp
// Final NaN/Inf guard
if (! std::isfinite (sample))
{
    sample = 0.0f;
    filterBank.reset();
    consonantEngine.reset();
}
```

### Layer C: Consonant Output Cap (ConsonantEngine.h)

In `getNextSample()`, before the final `return output;` (line 125), add a hard clamp:

```cpp
return juce::jlimit(-1.0f, 1.0f, output);
```

This replaces:
```cpp
return output;
```

Note: add `#include <JuceHeader.h>` is already present at the top of this file, so `juce::jlimit` is available.

### Layer D: Brickwall Limiter in processBlock (PluginProcessor.cpp)

In `processBlock()`, after the output gain smoothing loop (after line 385), add a final brickwall hard clip:

```cpp
// Brickwall safety limiter — hard clip at 0 dBFS
// This is the last line of defense against dangerous output levels
for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
{
    auto* channelData = buffer.getWritePointer(ch);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        channelData[i] = juce::jlimit(-1.0f, 1.0f, channelData[i]);
}
```

Insert this block just before the closing `}` of processBlock, after the existing gain loop.

## Version

PATCH: 1.1.0 -> 1.1.1 (safety fix, no feature changes)

## Files Modified

1. `Source/dsp/FormantFilterBank.h` — Q clamping + bandwidth scaling (Layer A)
2. `Source/FormantVoice.cpp` — per-voice soft clip replacing NaN-only guard (Layer B)
3. `Source/dsp/ConsonantEngine.h` — consonant output hard clamp (Layer C)
4. `Source/PluginProcessor.cpp` — brickwall limiter after output gain (Layer D)

## Testing

- Play notes with formant shift at +24st, spread at 2.0, auto-consonant ON, consonant level at 1.0, manner at 0 (plosive). This is the worst-case scenario. Output should never exceed 0 dBFS.
- Play 16-note chord with same extreme settings. Should remain at safe levels.
- Verify normal playing (default settings, reasonable shift/spread values) still sounds correct — the tanh soft clip should be transparent at normal levels since the signal is already near [-1, 1] range.
- Run pluginval at strictness level 10.
