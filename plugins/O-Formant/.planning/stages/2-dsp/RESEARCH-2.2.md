# Stage 2 DSP: Phase 2.2 Modulation & Expression - Research

**Researched:** 2026-04-05
**Domain:** DSP -- Consonant noise synthesis, vibrato LFO, pitch glide, MPE integration
**Confidence:** HIGH

## Summary

Phase 2.2 adds four DSP components to the existing FormantVoice: ConsonantEngine (KLATT-inspired noise branch), VibratoLFO (sine + micro-jitter), PitchGlide (exponential smoother), and MPE expression mapping. The existing codebase already has proven patterns for all core techniques: O-Prism's `GlideProcessor` demonstrates the exact one-pole exponential smoother needed for PitchGlide, O-Formant's `AspirationNoise` shows per-voice `juce::Random` for decorrelated noise, and `FormantBiquad` provides the biquad filter pattern reusable for consonant tone shaping.

JUCE 8.0.4's MPE API has been verified directly from source. Key finding: `MPEValue::asUnsignedFloat()` returns 0.0-1.0 (use for pressure/breathiness), while `asSignedFloat()` returns -1.0 to 1.0 (use for timbre/slide). In legacy mode (currently enabled with pitchbend range 2), pressure works via channel pressure messages, and timbre works via CC74 -- both routed through the same `notePressureChanged()`/`noteTimbreChanged()` callbacks. The `currentlyPlayingNote` member is updated by the synthesiser before calling the voice callback, so reading it in the callback is correct.

**Primary recommendation:** Implement all 4 components as header-only classes following the existing DSP header pattern (LFGlottalSource, AspirationNoise). Reuse `FormantBiquad` for the ConsonantEngine's LP/HP/sibilance filters. Use the existing O-Prism `GlideProcessor` coefficient formula for PitchGlide.

---

## 1. ConsonantEngine Findings

### KLATT Dual-Branch Noise Topology

The KLATT model uses parallel noise branches shaped by filters to create different consonant sounds. The implementation for O-Formant simplifies this into:

1. **White noise source** (per-voice `juce::Random`, already proven in `AspirationNoise`)
2. **LP branch** (~2kHz cutoff) for dark consonants (/f/, /v/, /th/)
3. **HP branch** (~6kHz cutoff) for bright consonants (/s/, /sh/)
4. **Crossfade** controlled by `consonantTone` parameter (0.0 = LP only, 1.0 = HP only)
5. **Sibilance resonance** (bandpass at 4-8kHz) added on top

### Filter Implementation

Reuse `FormantBiquad` (Direct Form II Transposed, already includes NaN guard). Need 3 biquad instances per ConsonantEngine:

```cpp
// LP filter for dark consonants
auto lpCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sampleRate, 2000.0f, 0.707f);
lpFilter.setCoefficients (lpCoeffs);

// HP filter for bright consonants
auto hpCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sampleRate, 6000.0f, 0.707f);
hpFilter.setCoefficients (hpCoeffs);

// Sibilance bandpass resonance
float sibilanceFreq = 5500.0f; // Center of 4-8kHz range
float sibilanceQ = 2.0f + sibilance * 8.0f; // Q range: 2-10
auto bpCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sampleRate, sibilanceFreq, sibilanceQ);
sibilanceFilter.setCoefficients (bpCoeffs);
```

**Sibilance Q values:**
- Q = 2-3: Broad, gentle emphasis (subtle /s/)
- Q = 4-6: Clear sibilance, musical range
- Q = 8-10: Sharp, aggressive sibilance
- Recommendation: Map sibilance param 0.0-1.0 to Q 2.0-10.0

**Sibilance frequency:** The center frequency could be fixed at ~5.5kHz or mapped with the sibilance parameter. Real sibilants peak around 4-8kHz. A fixed center with Q variation is simpler and sounds natural.

### Tone Crossfade Implementation

```cpp
float noise = random.nextFloat() * 2.0f - 1.0f;
float lpOut = lpFilter.processSample (noise);
float hpOut = hpFilter.processSample (noise);

// Linear crossfade by consonantTone
float tonedNoise = (1.0f - consonantTone) * lpOut + consonantTone * hpOut;

// Add sibilance on top (additive, not replacing)
float sibilantOut = sibilanceFilter.processSample (noise);
float consonantOut = tonedNoise + sibilance * sibilantOut;
```

### Auto-Consonant Plosive Burst

**Envelope:** Falling exponential over 10-25ms, controlled by a sample counter.

```cpp
// In noteStarted():
burstSamplesRemaining = static_cast<int> (burstDurationMs * 0.001f * sampleRate);
burstAmplitude = velocity; // Velocity scales burst level

// In per-sample processing:
if (burstSamplesRemaining > 0)
{
    float burstProgress = 1.0f - static_cast<float> (burstSamplesRemaining) / burstTotalSamples;
    float burstEnv = std::exp (-5.0f * burstProgress); // Fast exponential decay
    float burstSample = consonantOut * burstEnv * burstAmplitude;
    // Add burst to output
    --burstSamplesRemaining;
}
```

**Burst duration:** Use a fixed ~15ms (midpoint of 10-25ms). At 44.1kHz this is ~661 samples. The exponential envelope with decay constant of -5.0 gives natural plosive character (amplitude drops to ~0.7% by end of burst).

**Crossfade with ADSR attack:** The burst plays during the first 15ms of the note. The ADSR attack ramp naturally crossfades with the decaying burst -- no special crossfade logic needed. If the ADSR attack time is very short (<15ms), the burst will be masked by the onset of the vowel, which is musically correct (fast attack = less consonant).

### Early-Out Optimization

```cpp
// Skip entire consonant branch when not needed
bool consonantActive = (consonantLevel > 0.001f) || (autoConsonant && burstSamplesRemaining > 0);
if (!consonantActive)
    return; // Or skip consonant processing in renderNextBlock
```

### Per-Voice Random Decorrelation

Follow the `AspirationNoise` pattern: seed = `voiceIndex * 37 + 23` (different prime offset from AspirationNoise's `voiceIndex * 31 + 17`).

**Confidence: HIGH** -- FormantBiquad and ArrayCoefficients verified from JUCE 8.0.4 source, AspirationNoise pattern proven in Phase 2.1.

---

## 2. VibratoLFO Findings

### Phase Accumulator Approach

Per-voice sine LFO using a double-precision phase accumulator (matching LFGlottalSource pattern):

```cpp
class VibratoLFO
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        phase = 0.0;
        delayCounter = 0;
    }

    void noteOn (float delayMs) noexcept
    {
        phase = 0.0;
        delayCounter = 0;
        delaySamples = static_cast<int> (delayMs * 0.001f * static_cast<float> (sampleRate));
        prevSinSign = false; // Track zero-crossings for jitter
        jitterApplied = false;
    }

    // Returns pitch modulation in cents
    float getNextValue (float rateHz, float depthCents) noexcept
    {
        // Onset delay ramp
        float delayGain = 1.0f;
        if (delayCounter < delaySamples)
        {
            delayGain = static_cast<float> (delayCounter) / static_cast<float> (delaySamples);
            ++delayCounter;
        }

        // Phase accumulator
        double phaseInc = static_cast<double> (rateHz) / sampleRate;
        phase += phaseInc;

        // Detect zero-crossing (positive-going) for micro-jitter
        float sinVal = std::sin (static_cast<float> (phase * juce::MathConstants<double>::twoPi));
        bool currentSign = sinVal >= 0.0f;
        if (currentSign && !prevSinSign)
        {
            // New cycle: apply micro-jitter
            jitterOffset = (random.nextFloat() - 0.5f) * 0.01f; // +/-0.5% = +/-0.005
        }
        prevSinSign = currentSign;

        // Wrap phase
        if (phase >= 1.0)
            phase -= 1.0;

        return depthCents * sinVal * delayGain;
    }

    float getJitterOffset() const noexcept { return jitterOffset; }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    int delayCounter = 0;
    int delaySamples = 0;
    bool prevSinSign = false;
    float jitterOffset = 0.0f;
    juce::Random random;
};
```

### Onset Delay

Linear ramp from 0 to 1 over `vibratoDelay` ms. This is a simple counter-based approach -- no SmoothedValue needed since the ramp itself is the smoothing. The delay counter increments each sample until it reaches `delaySamples`, at which point `delayGain` saturates at 1.0.

### Micro-Jitter Implementation

**+/-0.5% F0 perturbation:** This means the fundamental frequency is randomly offset by up to 0.5% each vibrato cycle.

- Detect zero-crossing of the sine wave (positive-going)
- On each crossing, generate a new random offset in range [-0.005, +0.005]
- Apply this offset to F0 as a multiplier: `f0 * (1.0 + jitterOffset)`

**Why per-cycle, not per-sample:** Per-sample jitter at such low amplitude would be inaudible (filtered by the oscillator). Per-cycle jitter causes slight cycle-to-cycle frequency variation, which is what makes natural voices sound alive. This matches how vocal jitter actually works (cycle-to-cycle F0 perturbation is a well-studied acoustic measure).

### Applying Vibrato to F0

```cpp
// In renderNextBlock, per-sample:
float vibratoCents = vibratoLFO.getNextValue (vibratoRate, vibratoDepth);
float jitter = vibratoLFO.getJitterOffset();
float modulatedF0 = baseF0 * std::pow (2.0f, vibratoCents / 1200.0f) * (1.0f + jitter);
```

The `pow(2, cents/1200)` conversion is the standard pitch-to-frequency formula. For small cent values (typical vibrato is 10-50 cents), this is numerically stable. No need for a fast approximation -- `std::pow` on modern CPUs with these magnitudes is fast enough.

**Confidence: HIGH** -- Phase accumulator pattern matches LFGlottalSource, jitter technique is well-documented in speech synthesis literature.

---

## 3. PitchGlide Findings

### One-Pole Exponential Smoother

The O-Prism `GlideProcessor` provides the exact pattern needed. The coefficient formula:

```cpp
coeff = std::exp (-1.0 / (glideTime * sampleRate));
```

This gives a one-pole lowpass where the time constant equals `glideTime` seconds (time to reach ~63.2% of target). Per-sample update:

```cpp
currentFreq = currentFreq * coeff + targetFreq * (1.0 - coeff);
```

### Simplified Version for O-Formant

The O-Prism `GlideProcessor` has mode-switching (Off/Legato/Always) that O-Formant does not need. The O-Formant version is simpler:

```cpp
class PitchGlide
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
    }

    void setTarget (float freqHz) noexcept
    {
        targetFreq = freqHz;
    }

    void snapTo (float freqHz) noexcept
    {
        currentFreq = freqHz;
        targetFreq = freqHz;
    }

    void setTime (float timeMs) noexcept
    {
        glideTimeMs = timeMs;
        if (timeMs > 0.0f && sampleRate > 0.0)
            coeff = static_cast<float> (std::exp (-1.0 / (static_cast<double> (timeMs) * 0.001 * sampleRate)));
        else
            coeff = 0.0f; // Instant snap
    }

    float getNextFrequency() noexcept
    {
        if (coeff <= 0.0f || std::abs (currentFreq - targetFreq) < targetFreq * 0.00001f)
        {
            currentFreq = targetFreq;
            return currentFreq;
        }

        currentFreq = currentFreq * coeff + targetFreq * (1.0f - coeff);
        return currentFreq;
    }

private:
    double sampleRate = 44100.0;
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
    float glideTimeMs = 0.0f;
    float coeff = 0.0f;
};
```

### Edge Cases

**pitchGlide == 0:** When glide time is 0, `coeff` is 0 and `getNextFrequency()` returns `targetFreq` immediately. The threshold check (`< targetFreq * 0.00001f`) also catches this.

**Voice steal / reassign:** When a voice is stolen, `noteStarted()` fires with a new note. The key behavior:
- If glide is enabled: DON'T call `snapTo()`. Just call `setTarget(newFreq)`. The voice will glide from its current (old) frequency to the new one.
- If glide is disabled: Call `snapTo(newFreq)` to jump immediately.
- On the very first note of a voice (no prior pitch), call `snapTo()` to avoid gliding from 440Hz.

```cpp
// In noteStarted():
float f0 = static_cast<float> (getCurrentlyPlayingNote().getFrequencyInHertz());
float glideMs = pPitchGlide->load();

pitchGlide.setTime (glideMs);

if (glideMs > 0.0f && wasActive) // Had a prior note
    pitchGlide.setTarget (f0); // Glide from old to new
else
    pitchGlide.snapTo (f0); // Jump immediately
```

**wasActive tracking:** Add a `bool wasActive = false;` member to FormantVoice. Set to `true` at end of `noteStarted()`, reset to `false` in `noteStopped()` only when the voice fully clears (not during tail-off).

### Frequency Domain vs. Log Domain

The one-pole smoother operates in linear frequency space. This means the glide rate is not perceptually constant (higher pitches glide faster in cents). For a vocal synth where glides are typically within an octave, this is acceptable and matches the O-Prism implementation. A log-frequency smoother would be more perceptually correct but adds complexity (log/exp per sample) -- not worth it for v1.

**Confidence: HIGH** -- Direct code reference from O-Prism GlideProcessor, verified locally.

---

## 4. MPE Integration Findings

### JUCE 8.0.4 MPE API (Verified from Source)

**MPESynthesiserVoice callbacks:**
- `notePressureChanged()` -- called when per-note pressure changes
- `noteTimbreChanged()` -- called when per-note timbre (CC74 / slide) changes
- `notePitchbendChanged()` -- called when per-note pitchbend changes
- `noteKeyStateChanged()` -- called when key state changes (sustain/sostenuto)

**Accessing note data in callbacks:**

The MPESynthesiser updates `currentlyPlayingNote` BEFORE calling the voice callback. Verified from `juce_MPESynthesiser.cpp`:

```cpp
// MPESynthesiser::notePressureChanged calls:
voice->currentlyPlayingNote = changedNote; // Updated first
voice->notePressureChanged();              // Then callback fires
```

So this is safe:
```cpp
void FormantVoice::notePressureChanged()
{
    auto note = getCurrentlyPlayingNote();
    float pressure = note.pressure.asUnsignedFloat(); // 0.0 to 1.0
    // Use pressure...
}
```

### MPEValue API (Verified from Source)

| Method | Range | Use Case |
|--------|-------|----------|
| `asUnsignedFloat()` | 0.0 to 1.0 | Pressure (0=none, 1=full). Breathiness mapping. |
| `asSignedFloat()` | -1.0 to 1.0 | Pitchbend, Timbre/slide (bipolar). Vowel Y offset. |
| `as7BitInt()` | 0 to 127 | Low-resolution integer value |
| `as14BitInt()` | 0 to 16383 | Full-resolution integer value |

**Critical distinction for this plugin:**
- **Pressure -> Breathiness:** Use `asUnsignedFloat()`. Pressure is unipolar (0 = no pressure, 1 = max). Default value at note-on is `MPEValue::minValue()` = 0.0 unsigned.
- **Timbre -> Vowel Y:** Use `asUnsignedFloat()`. Although timbre CAN be bipolar, for Vowel Y modulation (0-1 range) it makes more sense to use unsigned. Default is `centreValue()` = 0.5 unsigned.

**Velocity:** `note.noteOnVelocity.asUnsignedFloat()` gives 0.0-1.0.

### Legacy Mode Behavior (CRITICAL)

The plugin currently uses:
```cpp
synthesiser.enableLegacyMode (2, juce::Range<int> (1, 17));
```

In legacy mode, JUCE's `MPEInstrument` maps standard MIDI to MPE dimensions:
- **Channel pressure** (MIDI message) -> `pressure` dimension -> triggers `notePressureChanged()`
- **CC74** (MIDI controller) -> `timbre` dimension -> triggers `noteTimbreChanged()`
- **Pitchbend** (MIDI message) -> `pitchbend` dimension -> triggers `notePitchbendChanged()`
- **CC70** -> alternate pressure (14-bit with CC102 as LSB)
- **CC74** -> timbre (14-bit with CC106 as LSB)

Verified from JUCE source unit tests (juce_MPEInstrument.cpp:1835-1887):
- `channelPressureChange` -> pressure callback
- `controllerEvent(ch, 74, val)` -> timbre callback

**This means:** In legacy mode, standard MIDI aftertouch maps to pressure, and CC74 (Brightness) maps to timbre. Both work through the exact same voice callbacks. The integration code does not need to differentiate between MPE and legacy mode -- the callbacks fire in both cases.

**Important caveat for legacy mode:** In legacy mode, all notes on the same MIDI channel share dimension values. If two notes are on channel 1, a pressure change on channel 1 updates BOTH notes' pressure values. In true MPE mode, each note has its own channel and thus independent dimensions.

### Per-Voice MPE State

Add to FormantVoice:
```cpp
// Per-voice MPE modulation state
float mpeBreathOffset = 0.0f;  // From pressure
float mpeVowelYOffset = 0.0f;  // From timbre
float noteVelocity = 0.0f;     // Stored at note-on
```

### Pressure -> Breathiness Mapping

From CONTEXT-2.2.md: "Effective breathiness = knob_value + pressure * (1.0 - knob_value)"

```cpp
void FormantVoice::notePressureChanged()
{
    float pressure = getCurrentlyPlayingNote().pressure.asUnsignedFloat();
    mpeBreathOffset = pressure;
}

// In renderNextBlock:
float knobBreath = pBreathiness->load();
float effectiveBreath = knobBreath + mpeBreathOffset * (1.0f - knobBreath);
aspirationNoise.setBreathiness (effectiveBreath);
```

This formula ensures:
- At pressure 0: breathiness = knob value (no change)
- At pressure 1: breathiness = 1.0 (maximum, regardless of knob)
- The knob sets a floor, pressure only adds

### Timbre -> Vowel Y Mapping

```cpp
void FormantVoice::noteTimbreChanged()
{
    // Timbre default is centreValue (0.5 unsigned), so subtract centre
    float timbre = getCurrentlyPlayingNote().timbre.asUnsignedFloat();
    mpeVowelYOffset = timbre - 0.5f; // Range: -0.5 to +0.5
}

// In renderNextBlock (block-rate coefficient update):
float vowelY = pVowelY->load() + mpeVowelYOffset;
vowelY = juce::jlimit (0.0f, 1.0f, vowelY);
```

### Velocity -> Attack Character

```cpp
void FormantVoice::noteStarted()
{
    noteVelocity = getCurrentlyPlayingNote().noteOnVelocity.asUnsignedFloat();
    // Pass velocity to ConsonantEngine for burst amplitude scaling
    consonantEngine.triggerBurst (noteVelocity);
}
```

**Confidence: HIGH** -- All API details verified directly from JUCE 8.0.4 source at `/Users/taylorbrook/JUCE/modules/juce_audio_basics/mpe/`.

---

## 5. Existing Codebase Patterns

### Patterns to Reuse

| Pattern | Source | Reuse In |
|---------|--------|----------|
| Per-voice `juce::Random` with decorrelated seeds | `AspirationNoise` (seed = `voiceIndex * 31 + 17`) | ConsonantEngine noise source |
| `FormantBiquad` (DF2T + NaN guard, 32 bytes) | `FormantBiquad.h` | ConsonantEngine LP/HP/sibilance filters |
| `ArrayCoefficients::makeBandPass/makeLowPass/makeHighPass` | `FormantFilterBank.h` | ConsonantEngine coefficient computation |
| One-pole exponential smoother | O-Prism `GlideProcessor` | PitchGlide (direct adaptation) |
| Phase accumulator (double precision) | `LFGlottalSource` | VibratoLFO |
| `SmoothedValue<float>` for parameter smoothing | `AspirationNoise` breathiness | ConsonantEngine consonantLevel smoothing |
| Cached APVTS parameter pointers | `FormantVoice.h` (21 parameters) | Already cached, no new pattern needed |

### Header-Only DSP Class Convention

All DSP components in O-Formant follow a header-only pattern:
- `prepare(double sampleRate)` for initialization
- `reset()` to clear state
- Inline hot-path processing methods
- No heap allocations
- Members initialized in-class

New files should follow this exact convention: `VibratoLFO.h`, `PitchGlide.h`, `ConsonantEngine.h`.

### Integration Point in renderNextBlock

Current signal flow in `FormantVoice::renderNextBlock()`:
```
F0 from MPENote -> glottalSource.setFrequency
glottalSource.getNextSample -> aspirationNoise.process -> filterBank.process -> ADSR -> output
```

New signal flow:
```
F0 from MPENote
  -> pitchGlide.getNextFrequency      [NEW]
  -> vibratoLFO.getNextValue           [NEW]
  -> glottalSource.setFrequency

glottalSource.getNextSample -> aspirationNoise.process -> filterBank.process -> formantOut

consonantEngine.getNextSample          [NEW]

Mix: formantOut + consonantLevel * consonantOut
  -> ADSR -> output
```

The key integration changes in the per-sample loop:
1. Replace the direct `glottalSource.setFrequency(f0)` (currently block-rate) with per-sample frequency from PitchGlide + VibratoLFO
2. Add consonant branch processing after formant filtering
3. Mix formant and consonant outputs before ADSR envelope

**Note:** Moving F0 updates from block-rate to per-sample is needed for smooth vibrato. The `glottalSource.setFrequency()` call just updates a phase increment -- it's trivially cheap.

---

## 6. Risks and Recommendations

### Risk 1: Filter Instability in ConsonantEngine

**What could go wrong:** The LP, HP, and sibilance filters process white noise with potentially high Q values. Rapid parameter changes during automation could cause transient instability.

**Mitigation:** The `FormantBiquad` already has a NaN/Inf guard that resets state on detection. Additionally, update ConsonantEngine filter coefficients at block-rate (every 32 samples, matching `kCoeffUpdateInterval`) rather than per-sample. Clamp sibilance Q to max 10.0.

### Risk 2: Plosive Burst Clicks

**What could go wrong:** The auto-consonant burst is a sudden onset of noise that could produce audible clicks, especially at high consonantLevel values.

**Mitigation:** Start the burst envelope at a value slightly below 1.0 (e.g., 0.95) with a 1-2ms fade-in (just ~44-88 samples). The exponential decay handles the tail naturally. If clicks persist, add a 2-sample crossfade at burst onset.

### Risk 3: Vibrato Phase Discontinuity on Note Steal

**What could go wrong:** When a voice is stolen and restarted, the vibrato LFO resets phase to 0, but the onset delay means vibrato won't resume for another delay period.

**Mitigation:** This is actually correct behavior -- each new note should have its own vibrato onset delay. The reset in `noteOn()` is the right approach. If seamless steal is desired later, the vibrato could be made continuous (don't reset phase), but for v1 the per-note reset matches natural vocal behavior.

### Risk 4: CPU Impact of Per-Sample F0 Update

**What could go wrong:** Moving `glottalSource.setFrequency(f0)` from block-rate (every 32 samples) to per-sample increases computation by 32x for that specific operation.

**Mitigation:** `setFrequency()` is just a division and assignment (`phaseIncrement = f0 / sampleRate`). At ~1 FLOP per sample, this is negligible even for 16 voices. No concern.

### Risk 5: Legacy Mode Sharing

**What could go wrong:** In the current legacy mode configuration, all 16 MIDI channels are in the channel range. If a DAW sends notes on the same channel, pressure/timbre changes affect ALL notes on that channel simultaneously. This may surprise users expecting per-note expression.

**Recommendation:** Document this as a known limitation of legacy mode. For true per-note expression, users should switch to MPE mode in their DAW/controller. The code itself handles both modes identically -- no special casing needed.

### Recommendation: Processing Order

The signal flow order matters for natural sound:

1. **PitchGlide first** -- Smooths the target frequency
2. **VibratoLFO second** -- Modulates the already-glided frequency
3. **F0 to glottal source** -- With both glide and vibrato applied

This order ensures vibrato modulates around the current (possibly gliding) pitch, which sounds natural. Reversing the order (vibrato then glide) would smooth out the vibrato modulation, which is wrong.

### Recommendation: ConsonantEngine Before ADSR

The consonant output should be envelope-gated by the same ADSR as the formant output:
```
finalSample = (formantOut + consonantLevel * consonantOut) * adsrEnvelope;
```

This ensures consonant noise fades with the note release, preventing noise tails after note-off.

### Recommendation: File Structure

```
Source/dsp/
  VibratoLFO.h          # ~80 lines, header-only
  PitchGlide.h          # ~60 lines, header-only
  ConsonantEngine.h     # ~120 lines, header-only, uses FormantBiquad
```

No .cpp files needed. All three classes are small enough for header-only and benefit from inline hot paths.
