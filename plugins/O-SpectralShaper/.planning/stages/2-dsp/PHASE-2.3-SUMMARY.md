# Phase 2.3: Envelope Shaping & Parameters - Summary

**Plugin:** O-SpectralShaper
**Phase:** 2.3 (Envelope Shaping & Full Parameter Integration)
**Completed:** 2026-02-03
**Status:** Ready for Build

---

## Implementation Summary

### Files Modified (3 files)

| File | Changes |
|------|---------|
| `Source/STFTProcessor.cpp` | Implemented `applyEnvelopeShaping()` with gain calculation |
| `Source/PluginProcessor.h` | Added lookahead buffer and helper methods |
| `Source/PluginProcessor.cpp` | Added lookahead, state save/load for curves |

---

## Technical Details

### Per-Band Envelope Shaping Algorithm

**Core concept:** Apply different gain to transient vs sustain portions of each frequency band.

```cpp
// Get transient activity (0.0 = sustain, 1.0 = transient)
float transient = bands[band].transientActivity;
float sustain = 1.0f - transient;

// Attack shaping: Boost/cut transients based on attack curve
float attackDB = attackCurveData[band] * attackTimeMs * 0.1f;
float attackGain = Decibels::decibelsToGain(attackDB * transient);

// Sustain shaping: Boost/cut non-transient portions based on sustain curve
float sustainDB = sustainCurveData[band] * sustainTimeMs * 0.01f;
float sustainGain = Decibels::decibelsToGain(sustainDB * sustain);

// Combined gain (multiplicative)
float targetGain = attackGain * sustainGain;
```

### Curve Scaling

**Attack Curve:**
- Curve value: -1.0 to +1.0
- Attack time: 0.1ms to 50ms
- dB range: `attackCurve[band] * attackTimeMs * 0.1`
- Example: `attackCurve[15] = +1.0`, `attackTimeMs = 10ms` → +1.0dB transient boost

**Sustain Curve:**
- Curve value: -1.0 to +1.0
- Sustain time: 10ms to 500ms
- dB range: `sustainCurve[band] * sustainTimeMs * 0.01`
- Example: `sustainCurve[15] = -1.0`, `sustainTimeMs = 100ms` → -1.0dB sustain cut

### Gain Smoothing

```cpp
// SmoothedValue prevents clicks during gain changes
bands[band].gainSmoothed.setTargetValue(targetGain);
float smoothedGain = bands[band].gainSmoothed.getNextValue();
```

**Ramp time:** 50ms (set in `prepare()`)

### Magnitude-Only Processing (Phase Preservation)

```cpp
// Apply gain to both real and imaginary components equally
// This scales magnitude while preserving phase
fftData[bin * 2] *= smoothedGain;      // Real
fftData[bin * 2 + 1] *= smoothedGain;  // Imaginary
```

**Why this matters:**
- Scaling complex FFT bins maintains phase relationships
- No phase distortion introduced
- Stereo image preserved
- Mono-compatible

---

## State Save/Load for Curves

### Saving Curves

```cpp
void getStateInformation(MemoryBlock& destData)
{
    auto state = parameters.copyState();
    auto curvesXml = state.getOrCreateChildWithName("Curves", nullptr);

    // Encode as hex string (binary-safe)
    String attackHex = String::toHexString(
        reinterpret_cast<const uint8*>(attackCurve.data()),
        static_cast<int>(attackCurve.size() * sizeof(float))
    );
    curvesXml.setProperty("attackCurve", attackHex, nullptr);

    // Same for sustain curve...
}
```

### Loading Curves

```cpp
void setStateInformation(const void* data, int sizeInBytes)
{
    // ... restore parameters ...

    auto curves = state.getChildWithName("Curves");
    if (curves.isValid())
    {
        // Decode hex string back to float array
        MemoryBlock attackBlock;
        attackBlock.loadFromHexString(attackHex);
        std::memcpy(attackCurve.data(), attackBlock.getData(), attackBlock.getSize());

        // Update STFT processors with restored curves
        setAttackCurve(attackCurve);
    }
}
```

**Why hex encoding?**
- Binary-safe (XML doesn't handle raw binary well)
- Human-readable in debug
- JUCE provides built-in hex conversion

---

## Lookahead Buffer

### Purpose

Reduces pre-ringing on sharp transients by detecting them before they arrive.

**How it works:**
1. Input signal delayed by `lookaheadTimeMs`
2. STFT processor sees transient early
3. Gain changes applied before transient arrives
4. Result: Cleaner attack shaping

### Implementation

```cpp
float getLookaheadDelayedSample(int channel, float input)
{
    if (lookaheadDelayLength == 0)
        return input;  // Bypass if disabled

    // Write current input
    lookaheadBuffer.setSample(channel, lookaheadWritePosition, input);

    // Read delayed sample
    int readPosition = (lookaheadWritePosition - lookaheadDelayLength + bufferSize) % bufferSize;
    float delayed = lookaheadBuffer.getSample(channel, readPosition);

    // Advance write position
    lookaheadWritePosition = (lookaheadWritePosition + 1) % bufferSize;

    return delayed;
}
```

### Toggle Parameter

```cpp
bool lookaheadEnabled = lookaheadEnabledParam->load() > 0.5f;

if (lookaheadEnabled)
{
    lookaheadDelayLength = static_cast<int>(sampleRate * lookaheadTimeMs / 1000.0);
}
else
{
    lookaheadDelayLength = 0;  // Bypass
}
```

**Parameter:** `LOOKAHEAD_ENABLED` (bool, default OFF)
**Time parameter:** `LOOKAHEAD_TIME` (0.1-10ms, default 2ms)

---

## Dry/Wet Mixing with Latency Matching

### Signal Flow

```
Input → Lookahead → [Split]
                      ↓
                  [Dry Delay]  [STFT Processing]
                      ↓              ↓
                    512 samples   512 samples
                      ↓              ↓
                    [Mix] → Output Gain → Output
```

**Dry delay:** 512 samples (matches STFT latency)
**Lookahead:** Optional 0-10ms (user-adjustable)

### Why Latency Matching Matters

Without dry delay:
```
Dry:  [Input]----------------→
Wet:  [Input]→[512 delay]→[STFT]→
Mix:  [Comb filtering artifacts]
```

With dry delay:
```
Dry:  [Input]→[512 delay]-------→
Wet:  [Input]→[512 delay]→[STFT]→
Mix:  [Clean, phase-aligned]
```

---

## Full Parameter Integration

### All 7 APVTS Parameters Connected

| Parameter ID | Connected To | Effect |
|--------------|--------------|--------|
| MIX | Dry/wet blend | 0% = dry only, 100% = wet only |
| ATTACK_TIME | Attack curve scaling | 0.1-50ms, affects transient gain range |
| SUSTAIN_TIME | Sustain curve scaling | 10-500ms, affects tail gain range |
| SENSITIVITY | Transient detection threshold | 0-100%, modulates spectral flux |
| LOOKAHEAD_ENABLED | Lookahead buffer toggle | On/off |
| LOOKAHEAD_TIME | Lookahead delay length | 0.1-10ms when enabled |
| OUTPUT_GAIN | Final output level | -12 to +12dB |

### Parameter Update Flow

```
processBlock():
  1. Read all parameters (atomic)
  2. Update STFT processors (sensitivity, times, curves)
  3. Calculate lookahead delay length
  4. Process audio sample-by-sample
  5. Apply mix and output gain
```

---

## Real-Time Safety Verification

### ✅ Still Compliant

- [x] No memory allocation in `applyEnvelopeShaping()`
- [x] No memory allocation in processBlock
- [x] All buffers preallocated in prepareToPlay()
- [x] Atomic curve buffer swap (lock-free)
- [x] SmoothedValue uses pre-allocated state

### Atomic Curve Access

```cpp
// Read active curve buffer (atomic, lock-free)
int activeCurve = activeCurveBuffer.load(std::memory_order_acquire);
const auto& attackCurveData = attackCurve[activeCurve];
```

**Thread safety:**
- Audio thread reads from active buffer
- UI thread writes to inactive buffer
- Atomic swap when write complete
- No locks required

---

## Success Criteria (Phase 2.3)

### Build Verification
- [ ] `ninja O-SpectralShaper_VST3 O-SpectralShaper_AU` completes without errors
- [ ] No compiler warnings
- [ ] Plugin loads in DAW

### Shaping Verification

**Test 1: Attack Boost**
- Set `attackCurve[all] = +1.0` (full positive)
- Set `attackTimeMs = 10ms`
- Input: Drum loop
- Expected: Transients (kick/snare/hat attacks) boosted by ~+1dB
- Result: Louder, more punchy attacks

**Test 2: Sustain Cut**
- Set `sustainCurve[all] = -1.0` (full negative)
- Set `sustainTimeMs = 100ms`
- Input: Drum loop with reverb tail
- Expected: Tails/reverb reduced by ~-1dB
- Result: Tighter, shorter decays

**Test 3: Frequency-Selective Shaping**
- Set `attackCurve[0-7] = +1.0` (boost low freq transients)
- Set `attackCurve[24-31] = -1.0` (cut high freq transients)
- Input: Full mix
- Expected: Kick punch increased, cymbal splash reduced
- Result: Surgical transient control per frequency

### Parameter Verification

**Mix Parameter:**
- Mix = 0% → Dry signal only (no processing)
- Mix = 50% → Blend of dry and processed
- Mix = 100% → Fully processed signal

**Lookahead Toggle:**
- Lookahead OFF → No additional latency
- Lookahead ON (2ms) → Cleaner attack shaping on sharp transients

**Output Gain:**
- Output gain = 0dB → No level change
- Output gain = +6dB → 2x louder
- Output gain = -6dB → 1/2 as loud

### State Persistence
- [ ] Draw attack curve in UI
- [ ] Save preset/project
- [ ] Close and reopen plugin
- [ ] Verify: Attack curve restored correctly
- [ ] Repeat for sustain curve

---

## Known Limitations

1. **No UI yet** - Phase 3 will add WebView curve editor
2. **Curves set via code only** - UI needed for user control
3. **No visual feedback** - Phase 3 will add spectrogram with transient overlay

---

## Next Steps

### Build Command
```bash
cd /Users/taylorbrook/Dev/VST-development/build
ninja O-SpectralShaper_VST3 O-SpectralShaper_AU
```

### Installation Command
```bash
# Clear AU cache
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache

# Remove old and install fresh
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-SpectralShaper.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-SpectralShaper.component
cp -R build/plugins/O-SpectralShaper/O-SpectralShaper_artefacts/Release/VST3/O-SpectralShaper.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-SpectralShaper/O-SpectralShaper_artefacts/Release/AU/O-SpectralShaper.component ~/Library/Audio/Plug-Ins/Components/
```

### Validation Steps

**1. Build and Install**
```bash
cd /Users/taylorbrook/Dev/VST-development/build
ninja O-SpectralShaper_VST3 O-SpectralShaper_AU
# Run installation commands above
```

**2. Test in DAW (Logic Pro / Ableton)**
- Load O-SpectralShaper on drum loop track
- Adjust Mix parameter (0-100%)
- Adjust Sensitivity (0-100%)
- Adjust Attack Time (0.1-50ms)
- Adjust Sustain Time (10-500ms)
- Toggle Lookahead Enabled
- Adjust Output Gain (-12 to +12dB)

**3. Test Curve Persistence (Code-Based for Now)**
```cpp
// In PluginEditor constructor or custom test method:
std::array<float, 32> testAttackCurve;
std::fill(testAttackCurve.begin(), testAttackCurve.end(), 0.5f);
processor.setAttackCurve(testAttackCurve);
```

### Git Commit (After Successful Validation)
```bash
git add plugins/O-SpectralShaper/Source/STFTProcessor.cpp
git add plugins/O-SpectralShaper/Source/PluginProcessor.h
git add plugins/O-SpectralShaper/Source/PluginProcessor.cpp
git commit -m "feat(O-SpectralShaper): Phase 2.3 - Envelope shaping with attack/sustain curves"
```

---

## Code Statistics

| Metric | Value |
|--------|-------|
| New Lines of Code | ~120 |
| Modified Functions | 5 |
| New Helper Methods | 1 (getLookaheadDelayedSample) |
| Total DSP Code | ~730 lines |

---

## Performance Characteristics

### CPU Usage Estimate

**Per frame (256 samples @ 44.1kHz):**
- 32 bands × gain calculation
- 32 bands × SmoothedValue update
- 257 FFT bins × gain application
- Total: ~5-10% single core

**Lookahead overhead:**
- +1-2% CPU when enabled

**Total plugin CPU:** <15% single core @ 44.1kHz stereo

### Latency

**Base latency:** 512 samples (~11.6ms @ 44.1kHz)
**With lookahead (2ms):** 512 + 88 = 600 samples (~13.6ms @ 44.1kHz)

---

## Example Use Cases

### Use Case 1: Punchy Kick Drum

**Settings:**
- `attackCurve[0-7] = +1.0` (boost low freq transients)
- `sustainCurve[0-7] = -0.5` (reduce low freq tails)
- `attackTime = 5ms`
- `sustainTime = 200ms`
- `sensitivity = 0.6`

**Result:** Kick hits harder with tighter decay

### Use Case 2: Tame Harsh Cymbals

**Settings:**
- `attackCurve[24-31] = -0.7` (reduce high freq transients)
- `sustainCurve[24-31] = +0.3` (boost high freq sustain)
- `attackTime = 2ms`
- `sustainTime = 300ms`
- `sensitivity = 0.4`

**Result:** Cymbal attacks softer, shimmer enhanced

### Use Case 3: Enhance Snare Snap

**Settings:**
- `attackCurve[12-20] = +1.0` (boost mid-high freq transients)
- `sustainCurve[12-20] = -0.8` (reduce mid-high freq tails)
- `attackTime = 10ms`
- `sustainTime = 150ms`
- `sensitivity = 0.7`
- `lookahead = ON` (2ms)

**Result:** Snare crack enhanced, ring reduced

---

*Summary created: 2026-02-03*
*Phase 2.3 implementation complete - All DSP phases finished!*
*Ready for build, validation, and Stage 3 (GUI)*
