# Stage 2 DSP — Phase 3.3 Research: Multi-String + Sympathetic Coupling

**Plugin:** O-Bowed
**Phase:** 3.3 (Multi-String + Sympathetic Coupling)
**Date:** 2026-04-05
**Input:** CONTEXT-3.3.md (discuss phase), ARCHITECTURE.md, existing source code

---

## 1. Drone String Engine

### Architecture

Processor-level component owning 1-4 bowed string instances that run independently of MIDI. Each drone string reuses the existing DSP classes:
- `WaveguideString` — bidirectional delay waveguide
- `BowModel` — envelope-controlled excitation
- `HyperbolicFriction` — memoryless friction curve

**Why processor-level, not voice-level:** Drone strings are always active when STRING_COUNT >= 1. They don't respond to MIDI note-on/off — they just play continuously at their tuned pitches. This is architecturally identical to a sarangi or hurdy-gurdy drone.

### Pitch Model (resolves open question)

**Decision:** STRING_TUNING_1-4 are cents offsets from REFERENCE_PITCH (A4 = 440Hz default).

```cpp
float droneFreq = referencePitch * std::pow(2.0f, tuningCents / 1200.0f);
```

- 0 cents = A4 (440Hz)
- -1200 cents = A3 (220Hz)
- +700 cents = E5 (~659Hz, a fifth above A4)
- Range: ±2400 cents = ±2 octaves from reference

**Rationale:** Cents offset is the standard for tuning systems. Musicians think in intervals. This maps naturally to the existing REFERENCE_PITCH parameter which all tuning flows through.

### Per-String Randomization

Each drone string applies ±5% random variation to bow speed and bow pressure (from shared APVTS values). Variation is seeded per-string at prepare time (deterministic).

```cpp
// Per-string random offsets (fixed at prepare, not per-sample)
float speedVariation[4];   // e.g., [1.02, 0.97, 1.04, 0.98]
float pressureVariation[4]; // e.g., [0.97, 1.03, 0.99, 1.05]
```

**Rationale:** Real instruments have per-string mechanical variation even with identical bowing. ±5% is subtle enough to avoid pitch instability while adding natural character.

### Per-String Panning

Drone strings panned across stereo field based on string index:

| STRING_COUNT | String 1 | String 2 | String 3 | String 4 |
|-------------|----------|----------|----------|----------|
| 1 | center (0.5) | — | — | — |
| 2 | 0.35 (slight L) | 0.65 (slight R) | — | — |
| 3 | 0.25 | 0.50 | 0.75 | — |
| 4 | 0.20 | 0.40 | 0.60 | 0.80 |

Pan positions are fixed per configuration — no parameter needed. Equal-power panning:
```cpp
float gainL = std::cos(pan * halfPi);
float gainR = std::sin(pan * halfPi);
```

### Drone Bow Behavior

Drones are "always bowed" — they start in `prepareToPlay()` and stay active. Their BowModel is set to active with a default velocity of 0.5 (moderate bow). Speed/pressure track APVTS values (with per-string variation).

Key difference from voice bowing: no attack/release envelope. Drones sustain continuously. Use a constant `v_bow_target` without envelope ramping.

### Implementation Pattern

```cpp
class DroneStringEngine
{
    static constexpr int MAX_DRONES = 4;

    struct DroneString {
        WaveguideString waveguide;
        BowModel bow;
        HyperbolicFriction friction;
        float panL, panR;
        float speedVariation, pressureVariation;
    };

    std::array<DroneString, MAX_DRONES> drones;
    int activeCount = 0;
    // ...
};
```

---

## 2. Sympathetic String Engine (KS Waveguides)

### Karplus-Strong vs O-Lyrica Approach

**O-Lyrica's SympatheticResonanceEngine** uses bandpass resonator filters between active voices — mutual coupling where each voice's output excites resonance in others. This is computationally expensive (per-voice per-sample resonator processing + coupling matrix).

**O-Bowed needs a different architecture:** Passive KS waveguide strings that are excited by bridge output but don't feed back into the main waveguide. This is simpler, lower CPU, and physically accurate for sympathetic strings (which vibrate freely, not bowed).

### KS Waveguide Per Sympathetic String

Each sympathetic string is:
1. **Delay line** — `juce::dsp::DelayLine<float, Thiran>` sized for target pitch
2. **Loss filter** — one-pole lowpass controlling decay rate
3. **Excitation input** — attenuated bridge sum fed into delay line

```cpp
struct SympatheticString {
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> delay { 4410 };
    float lossCoeff = 0.999f;  // one-pole: y[n] = lossCoeff * y[n-1] + (1-lossCoeff) * x[n]
    float filterState = 0.0f;
    float frequency = 0.0f;
    float energyEstimate = 0.0f;
    bool active = false;
};
```

Per-sample processing:
```cpp
float processSample(float excitation) {
    float delayed = delay.popSample(0);
    // One-pole loss filter (controls decay time + brightness)
    filterState = lossCoeff * filterState + (1.0f - lossCoeff) * delayed;
    // Feed excitation + filtered feedback into delay
    delay.pushSample(0, filterState + excitation);
    return filterState;
}
```

**CPU estimate:** ~0.1-0.2% per string at 44.1kHz (one delay read + one filter multiply + one delay write). 12 strings ≈ 1.2-2.4%.

### Sympathetic Tuning (resolves open question)

**Decision:** Tune to harmonic partials of active voices. When multiple voices are active, distribute sympathetics across all voices' harmonic series, prioritizing lower-order harmonics.

Algorithm:
1. Collect fundamentals of all active voices (polyphonic + drone)
2. Generate candidate harmonics: fundamental, 2nd, 3rd, ... up to Nyquist
3. Sort candidates by harmonic order (lower = stronger coupling in physics)
4. Assign sympathetic strings to the top N candidates (N = SYMPATHETIC_COUNT)
5. Deduplicate near-matching frequencies (within 5 cents)

Example with 1 voice at A4 (440Hz), 6 sympathetics:
- 440Hz (1st), 880Hz (2nd), 1320Hz (3rd), 1760Hz (4th), 2200Hz (5th), 2640Hz (6th)

Example with 2 voices at A4 + E5 (659Hz), 6 sympathetics:
- 440, 659, 880, 1318, 1320, 1760 → deduplicate 1318/1320 → 440, 659, 880, 1319, 1760, 1978

### Retune Strategy (resolves open question)

**Decision:** Instant retune with crossfade. When a note changes:
1. New delay lengths calculated for new harmonic series
2. Over 64 samples, crossfade from old delay output to new delay output
3. No portamento — sympathetic strings should snap to new harmonics

**Rationale:** Real sympathetic strings don't glide — they either resonate or don't. Instant retune is physically correct. The 64-sample crossfade prevents clicks.

### Excitation Source (resolves open question)

**Decision:** Fixed 50/50 blend of pre-body (raw bridge sum) and post-body (resonated) signal.

```cpp
float preBody = bridgeSum;   // sum of all voice + drone bridge outputs
float postBody = bodyOut;    // after body resonator
float excitation = (preBody + postBody) * 0.5f * sympatheticAmount * 0.01f;
```

**Rationale:** Adding a blend parameter provides marginal musical value. The SYMPATHETIC_AMOUNT parameter already controls the overall level. Pre-body provides harmonically rich excitation; post-body adds body-colored resonance. 50/50 gives the best of both.

### Gating Optimization (resolves open question)

**Decision:** Amplitude-based gating.

```cpp
energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs(output);
bool shouldProcess = energyEstimate > 1e-7f || std::abs(excitation) > 1e-7f;
```

Skip `popSample`/`pushSample` entirely when gated — no CPU consumed. A string activates when excitation arrives and deactivates when energy decays below threshold.

**Rationale:** Matches WaveguideString.isActive() pattern. Energy-based is equivalent for single-frequency signals and simpler to implement.

### Sympathetic Panning

Sympathetic strings panned evenly across stereo field:
```cpp
float pan = (float)i / (float)(sympatheticCount - 1);  // 0.0 to 1.0
```

Blended with main signal at low level (SYMPATHETIC_AMOUNT scales the contribution).

---

## 3. Body Resonator Stereo Refactor

### Current State

`BodyResonator` processes mono (channel 0 only). It has 8 `IIR::Filter<float>` in parallel. The `process(float input)` method returns a single sample.

### Approach (resolves open question)

**Decision:** Shared coefficients, separate filter instances. Two arrays of 8 filters each.

```cpp
// Current (mono)
std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModes;

// New (stereo)
std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModesL;
std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModesR;
```

In `updateCoefficients()`, set the same coefficient pointer on both L and R:
```cpp
auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(...);
bodyModesL[i].coefficients = coeffs;
bodyModesR[i].coefficients = coeffs;
```

New API:
```cpp
void processStereo(float& left, float& right);  // replaces process(float)
```

**CPU impact:** +8 biquad evaluations per sample (was 8, now 16). Each biquad is ~5 multiplies + 5 adds. Total: ~80 additional FLOPs/sample. Negligible (~0.3% additional CPU).

**Why not process both channels with a single set of filters:** IIR filters have state (z1, z2). Each channel needs its own filter state to avoid crosstalk. Shared coefficients + separate state is the standard pattern for stereo biquad processing.

---

## 4. Dynamic Voice Cap

### Approach

Use a `maxPolyphony` variable in the processor. Before `synthesiser.renderNextBlock()`, check if active voice count exceeds the limit and force-stop excess voices.

```cpp
int maxPolyphony = 8;  // default

// Adjust based on drone count
switch (stringCount) {
    case 1: maxPolyphony = 8; break;
    case 2: maxPolyphony = 6; break;
    case 3: maxPolyphony = 5; break;
    case 4: maxPolyphony = 4; break;
}
```

**Implementation:** Before rendering, iterate voices. If active count > maxPolyphony, call `stopNote(0, false)` on the oldest voices (lowest voice index with active note).

**Why not add/remove voices from Synthesiser:** `addVoice()`/`removeVoice()` allocate/deallocate memory — not real-time safe. Keeping all 8 voice objects allocated and soft-limiting is cleaner.

**Alternative considered:** Let voice stealing handle it naturally. Rejected because the Synthesiser doesn't know about drone CPU — it would happily allocate 8 voices alongside 4 drones, exceeding CPU budget.

---

## 5. Voice Panning

### Current State

Voices write mono output equally to both stereo channels:
```cpp
for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
    outputBuffer.addSample(ch, startSample, sample);
```

### Change Required

Voices need per-voice pan position. Since voices are allocated by the Synthesiser (round-robin), assign pan based on voice index:

```cpp
// In renderNextBlock:
float panL = voicePanL;  // set in startNote or from processor
float panR = voicePanR;

outputBuffer.addSample(0, startSample, sample * panL);
outputBuffer.addSample(1, startSample, sample * panR);
```

For polyphonic voices (not drones), use a subtle spread:
- With STRING_COUNT=1: all voices center (current behavior maintained)
- With STRING_COUNT>1: voices get slight random pan variation (±10% from center)

This is a minor change to BowedStringVoice — add panL/panR members set from the processor.

---

## 6. Signal Flow (Phase 3.3)

```
processBlock:
  1. Read all params from APVTS (stringCount, tunings, sympathetic, bow, body, width)
  2. Enforce voice cap (stop excess voices if stringCount increased)
  3. synthesiser.renderNextBlock → polyphonic voices write panned stereo to buffer
  4. droneEngine.process → add panned drone output to buffer
  5. Save pre-body bridge sum (mono downmix of buffer for sympathetic excitation)
  6. bodyResonator.processStereo → process both L/R channels
  7. Compute sympathetic excitation = mix(preBody, postBody)
  8. sympatheticEngine.process → add sympathetic stereo output to buffer
  9. stereoWidthProcessor.processBlock → M/S width
  10. Apply output level
```

### Pre-Body Bridge Sum

Need to capture the bridge sum before body processing for sympathetic excitation:
```cpp
// Mono downmix before body resonator
float preBodySum = 0.0f;
for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    for (int i = 0; i < numSamples; ++i)
        preBodySum += buffer.getSample(ch, i);
// Actually, need per-sample, not block sum. Use a temporary mono buffer.
```

Better approach: process per-sample in a loop:
```cpp
for (int i = 0; i < numSamples; ++i) {
    float preBodyL = leftData[i];
    float preBodyR = rightData[i];
    float preBodyMono = (preBodyL + preBodyR) * 0.5f;

    bodyResonator.processStereo(leftData[i], rightData[i]);

    float postBodyMono = (leftData[i] + rightData[i]) * 0.5f;
    float excitation = (preBodyMono + postBodyMono) * 0.5f * sympatheticAmount * 0.01f;

    auto [sympL, sympR] = sympatheticEngine.processSample(excitation);
    leftData[i] += sympL;
    rightData[i] += sympR;
}
```

---

## 7. New Parameters Connected (Phase 3.3)

| Parameter | ID | Target |
|-----------|-----|--------|
| STRING_COUNT | `stringCount` | DroneStringEngine::setStringCount, voice cap |
| STRING_TUNING_1 | `stringTuning1` | DroneStringEngine::setTuning(0, cents) |
| STRING_TUNING_2 | `stringTuning2` | DroneStringEngine::setTuning(1, cents) |
| STRING_TUNING_3 | `stringTuning3` | DroneStringEngine::setTuning(2, cents) |
| STRING_TUNING_4 | `stringTuning4` | DroneStringEngine::setTuning(3, cents) |
| SYMPATHETIC_AMOUNT | `sympatheticAmount` | SympatheticStringEngine excitation level |
| SYMPATHETIC_COUNT | `sympatheticCount` | SympatheticStringEngine active string count |

**Parameters connected after Phase 3.3:** 17/22

---

## 8. New Source Files

| File | Purpose |
|------|---------|
| `Source/DSP/DroneStringEngine.h` | Drone engine header |
| `Source/DSP/DroneStringEngine.cpp` | Drone engine implementation |
| `Source/DSP/SympatheticStringEngine.h` | Sympathetic KS engine header |
| `Source/DSP/SympatheticStringEngine.cpp` | Sympathetic KS engine implementation |

**Modified files:**
- `Source/DSP/BodyResonator.h/.cpp` — stereo refactor
- `Source/PluginProcessor.h/.cpp` — integrate engines, new signal flow
- `Source/BowedStringVoice.h/.cpp` — per-voice panning
- `CMakeLists.txt` — add new source files

---

## 9. CPU Budget Analysis

| Component | Per-String CPU | Count | Total |
|-----------|---------------|-------|-------|
| Polyphonic voice (waveguide + friction) | ~2% | 4-8 | 8-16% |
| Drone string (waveguide + friction) | ~2% | 1-4 | 2-8% |
| Body resonator (16 biquads stereo) | — | 1 | ~1% |
| Sympathetic string (KS) | ~0.15% | 0-12 | 0-1.8% |
| Stereo width | — | 1 | ~0.1% |

**Worst case (4 drones + 4 voices + 12 sympathetics):**
~8% (drones) + ~8% (voices) + ~1% (body) + ~1.8% (sympathetic) + ~0.1% (width) = **~19%**

This is within the 25% budget from the ROADMAP. The dynamic voice cap (4 voices when 4 drones) keeps it manageable.

---

## 10. Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| Sympathetic feedback buildup | Medium | Soft-clip output, amplitude gating, coupling coefficient < 0.01 |
| Drone click on stringCount change | Medium | Crossfade drone on/off over 64 samples |
| Body resonator stereo crosstalk | Low | Separate filter instances per channel (standard pattern) |
| CPU spike on max configuration | Medium | Dynamic voice cap, sympathetic gating |
| Sympathetic retune click | Low | 64-sample crossfade on retune |
| Drone tuning discontinuity | Low | SmoothedValue for delay length changes |

---

## 11. Module Reuse

No external modules needed. All components are custom DSP built on JUCE primitives:
- `juce::dsp::DelayLine<float, Thiran>` — existing, used by WaveguideString and new KS strings
- `juce::dsp::IIR::Filter<float>` — existing, used by BodyResonator
- `WaveguideString`, `BowModel`, `HyperbolicFriction` — existing Phase 3.1 classes, reused by DroneStringEngine

O-Lyrica's `SympatheticResonanceEngine` is **not reusable** — it models mutual voice-to-voice coupling via bandpass resonators, while O-Bowed needs passive KS waveguides excited by bridge output. Different architecture, different purpose.

---

_Research complete. Ready for plan phase._
