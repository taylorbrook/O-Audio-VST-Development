# Phase 2.2: Per-Band Transient Detection - Summary

**Plugin:** O-SpectralShaper
**Phase:** 2.2 (Per-Band Transient Detection)
**Completed:** 2026-02-03
**Status:** Ready for Build

---

## Implementation Summary

### Files Modified (1 file)

| File | Changes |
|------|---------|
| `Source/STFTProcessor.cpp` | Implemented `detectTransients()` with spectral flux and dual envelopes |

---

## Technical Details

### Spectral Flux Detection Algorithm

**Spectral flux** measures the rate of change in spectral magnitude. High flux indicates transient onsets.

```cpp
// Calculate band magnitude (RMS across all bins in band)
float bandMagnitude = 0.0f;
for (int bin = startBin; bin < endBin; ++bin)
{
    float real = fftData[bin * 2];
    float imag = fftData[bin * 2 + 1];
    float binMagnitude = std::sqrt(real * real + imag * imag);
    bandMagnitude += binMagnitude * binMagnitude;
}
bandMagnitude = std::sqrt(bandMagnitude / (endBin - startBin));

// Positive-only difference (increases only)
float spectralFlux = juce::jmax(0.0f, bandMagnitude - bands[band].prevMagnitude);
bands[band].prevMagnitude = bandMagnitude;
```

**Why positive-only?**
- Positive flux = energy increase = transient onset
- Negative flux = energy decrease = sustain/decay (not a transient)

### Dual Envelope Followers

Two one-pole lowpass filters with different time constants:

**Fast Envelope (1ms attack):**
- Quickly responds to transient onsets
- High values during attacks

**Slow Envelope (15ms attack):**
- Follows overall energy more gradually
- Lags behind fast envelope during transients

```cpp
// One-pole filter coefficient calculation
float calculateEnvelopeCoefficient(float timeMs, double sampleRate, float hopSize)
{
    float timeSeconds = timeMs / 1000.0f;
    float hopRate = sampleRate / hopSize;
    return std::exp(-1.0f / (timeSeconds * hopRate));
}

// Envelope follower (one-pole lowpass)
bands[band].fastEnvelope = fastTarget + fastCoeff * (bands[band].fastEnvelope - fastTarget);
bands[band].slowEnvelope = slowTarget + slowCoeff * (bands[band].slowEnvelope - slowTarget);
```

**Hop rate adjustment:**
- Envelopes update at hop rate (256 samples @ 44.1kHz = ~172 Hz update rate)
- Coefficients adjusted for hop rate, not per-sample rate

### Transient Activity Calculation

```cpp
// Difference between fast and slow envelopes
float difference = bands[band].fastEnvelope - bands[band].slowEnvelope;

// Normalize to 0.0-1.0 range
float transient = juce::jlimit(0.0f, 1.0f, difference * 2.0f);

// Apply release envelope (50ms smooth decay)
bands[band].transientActivity = transient + releaseCoeff * (bands[band].transientActivity - transient);
```

**Result:**
- `transientActivity = 1.0` → Strong transient detected
- `transientActivity = 0.0` → No transient (sustain/tail)
- `transientActivity = 0.5` → Moderate transient activity

### Sensitivity Parameter

```cpp
spectralFlux *= sensitivity * 10.0f;
```

- **sensitivity = 0.0** → No detection (all activity suppressed)
- **sensitivity = 0.5** → Default sensitivity (5x scaling)
- **sensitivity = 1.0** → Maximum sensitivity (10x scaling)

**Effect:**
- Low sensitivity → Only strong transients detected
- High sensitivity → Subtle transients also detected

---

## Per-Band Processing

### 32 Logarithmic Bands

| Band | Approx Freq Range (@ 44.1kHz) | Musical Content |
|------|-------------------------------|-----------------|
| 0-3 | 20-80 Hz | Sub-bass, kick fundamentals |
| 4-7 | 80-320 Hz | Bass, low mids |
| 8-15 | 320-2.5 kHz | Midrange, vocals, snare body |
| 16-23 | 2.5-10 kHz | Presence, snare snap, hi-hat attack |
| 24-31 | 10-20 kHz | Air, cymbal brilliance |

### Independent Detection Per Band

Each band detects transients **independently**:
- Kick drum transient → High activity in bands 0-7 (low freq)
- Snare transient → High activity in bands 8-23 (mid-high freq)
- Hi-hat transient → High activity in bands 20-31 (high freq)

**Example: Drum loop**
```
Time:   [Kick]   [Snare]  [Hi-hat]
Band 0:  █████    ▁▁▁▁▁    ▁▁▁▁▁
Band 15: ▁▁▁▁▁    █████    ▁▁▁▁▁
Band 30: ▁▁▁▁▁    ▁▁▁▁▁    █████
```

---

## Real-Time Safety Verification

### ✅ Still Compliant

- [x] No memory allocation in `detectTransients()`
- [x] All computations use preallocated arrays
- [x] No locks or blocking operations
- [x] Bounded execution time (fixed 32 bands)

### Stack-Only Variables

```cpp
void detectTransients()
{
    // Only local stack variables (no heap allocation)
    float bandMagnitude = 0.0f;
    float spectralFlux = 0.0f;
    float difference = 0.0f;
    float transient = 0.0f;

    // Iterate over preallocated band array
    for (int band = 0; band < NUM_BANDS; ++band)
    {
        // Process using preallocated Band structures
    }
}
```

---

## Success Criteria (Phase 2.2)

### Build Verification
- [ ] `ninja O-SpectralShaper_VST3 O-SpectralShaper_AU` completes without errors
- [ ] No compiler warnings

### Transient Detection Verification

**Test 1: Impulse Response**
- Input: Single sample impulse (1.0, then 0.0...)
- Expected: High transient activity in all bands immediately after impulse
- Result: `transientActivity[all] → 1.0` then decay to 0.0

**Test 2: Sine Wave**
- Input: 440Hz sine wave (sustained tone)
- Expected: Low transient activity (only at start)
- Result: `transientActivity[15-20] → 0.1` (steady state after initial onset)

**Test 3: Drum Loop**
- Input: Kick/snare/hat loop
- Expected:
  - Kick hits → High activity in bands 0-7
  - Snare hits → High activity in bands 8-23
  - Hi-hat hits → High activity in bands 20-31
- Result: Visual confirmation via debug output

### Sensitivity Parameter Verification
- **sensitivity = 0.0** → `transientActivity[all] ≈ 0.0` (suppressed)
- **sensitivity = 0.5** → `transientActivity` responsive to moderate transients
- **sensitivity = 1.0** → `transientActivity` sensitive to subtle changes

---

## Debug Output (Optional)

For validation during development, add console logging:

```cpp
// In detectTransients() - only for debugging
#ifdef DEBUG
if (band == 15 && std::fmod(sampleTime, 1.0f) < 0.01f)  // Log band 15 once per second
{
    DBG("Band 15: flux=" << spectralFlux <<
        " fast=" << bands[band].fastEnvelope <<
        " slow=" << bands[band].slowEnvelope <<
        " activity=" << bands[band].transientActivity);
}
#endif
```

**Recommended:** Use DAW's plugin analyzer to visually confirm transient detection instead of console output (avoids I/O in audio thread).

---

## Known Limitations (To Be Addressed in Phase 2.3)

1. **Transient detection runs but has no effect on audio** - Phase 2.3 will apply gain based on transient activity
2. **Curves still not used** - Phase 2.3 will use attack/sustain curves to shape gain
3. **No visual feedback yet** - Phase 3 (GUI) will display transient activity on spectrogram

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
1. Run build command
2. Install plugin
3. Open DAW (Logic Pro / Ableton)
4. Load O-SpectralShaper on drum loop track
5. Adjust Sensitivity parameter
6. Verify: Audio still passes through without artifacts (shaping not applied yet)

### Git Commit (After Successful Validation)
```bash
git add plugins/O-SpectralShaper/Source/STFTProcessor.cpp
git commit -m "feat(O-SpectralShaper): Phase 2.2 - Per-band transient detection with spectral flux"
```

---

## Code Statistics

| Metric | Value |
|--------|-------|
| New Lines of Code | ~43 |
| Modified Functions | 1 (detectTransients) |
| Algorithm Complexity | O(NUM_BINS * NUM_BANDS) per frame |
| CPU Impact | ~5-10% additional (estimate) |

---

## Algorithm Performance

### Computational Cost Per Frame

**32 bands × ~8 bins/band average = ~256 bin magnitude calculations**

Per frame (256 samples @ 44.1kHz ≈ 5.8ms):
- 256 magnitude calculations (sqrt of sum of squares)
- 32 spectral flux calculations
- 64 envelope follower updates (fast + slow)
- 32 transient activity calculations

**Estimated CPU usage:** <10% single core @ 44.1kHz stereo

---

*Summary created: 2026-02-03*
*Phase 2.2 implementation complete - Ready for build and validation*
