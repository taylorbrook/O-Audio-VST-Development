# MFCC & Descriptor Extraction Pipeline - Research

**Researched:** 2026-02-14
**Domain:** JUCE 8.0.4 DSP (FFT, Windowing), MFCC computation, spectral descriptors
**Confidence:** HIGH (verified against JUCE 8.0.4 source code at `/Users/taylorbrook/JUCE`)
**Plugin:** O-TextureForge
**Stage:** 2 (DSP Implementation) - Phase 2.2

---

## Summary

This research covers the complete implementation details for extracting 19-dimensional descriptors (13 MFCCs + 6 spectral/temporal features) from audio grains using JUCE 8.0.4's built-in DSP classes. The critical findings are:

1. **JUCE FFT data layout is interleaved Complex<float>, NOT the pattern shown in ARCHITECTURE.md.** The output of `performRealOnlyForwardTransform` is `[real0, imag0, real1, imag1, ...]` -- pairs of (real, imaginary) values. The ARCHITECTURE.md code incorrectly uses `fftData[FFT_SIZE - k]` for imaginary parts.

2. **JUCE provides `performFrequencyOnlyForwardTransform`** which directly outputs magnitude values, avoiding manual complex-to-magnitude conversion entirely. This is the preferred approach for MFCC extraction.

3. **The `normalise` parameter in `WindowingFunction` should be `false` for MFCC analysis.** The default (`true`) normalizes the window to have unity DC gain, which changes the amplitude of the spectral analysis. For MFCC extraction, we want the standard Hann window without normalization.

4. **Pre-emphasis should be applied per-grain, not across the file.** Each grain is an independent analysis unit; applying pre-emphasis across grain boundaries would create artifacts at grain starts.

**Primary recommendation:** Use `performFrequencyOnlyForwardTransform` with `onlyCalculateNonNegativeFrequencies=true` for magnitude spectrum, and `WindowingFunction<float>(FFT_SIZE, hann, false)` for windowing. Build mel filterbank and DCT matrix once in `prepare()`, reuse for all grains.

---

## 1. juce::dsp::FFT - Verified API Details

**Source:** `/Users/taylorbrook/JUCE/modules/juce_dsp/frequency/juce_FFT.h` and `juce_FFT.cpp`
**Confidence:** HIGH (read directly from JUCE 8.0.4 source)

### Constructor

```cpp
juce::dsp::FFT fft { 11 };  // order = 11, size = 2^11 = 2048
```

The order parameter is `log2(fftSize)`. For FFT_SIZE = 2048, order = 11.

### performRealOnlyForwardTransform

```cpp
void performRealOnlyForwardTransform (float* inputOutputData,
                                      bool onlyCalculateNonNegativeFrequencies = false) const noexcept;
```

**Buffer requirements:**
- Input buffer must be `2 * getSize()` floats (i.e., 4096 floats for a 2048-point FFT)
- First half (indices 0..2047) contains the real input samples
- Second half (indices 2048..4095) should be zeroed before call

**Output data layout (CRITICAL - verified from source):**

The output is stored as **interleaved Complex<float>** -- the buffer is reinterpreted as `Complex<float>*`. Each complex number occupies 2 floats:

```
Index:  [0]    [1]    [2]    [3]    [4]    [5]    ...  [2k]     [2k+1]
Value:  real_0 imag_0 real_1 imag_1 real_2 imag_2 ...  real_k   imag_k
```

To extract magnitude at bin k:
```cpp
float real = fftData[2 * k];
float imag = fftData[2 * k + 1];
float magnitude = std::sqrt(real * real + imag * imag);
```

When `onlyCalculateNonNegativeFrequencies = false` (default): output contains `size` complex values (bins 0 through size-1). Negative frequencies are conjugate mirrors.

When `onlyCalculateNonNegativeFrequencies = true`: output contains at least `(size / 2) + 1` complex values (bins 0 through size/2). This is sufficient for MFCC extraction since we only need the positive frequency bins.

**WARNING: The ARCHITECTURE.md code has an INCORRECT magnitude extraction pattern:**
```cpp
// WRONG - from ARCHITECTURE.md
float real = fftData[k];
float imag = (k < FFT_SIZE/2) ? fftData[FFT_SIZE - k] : 0;
```
This treats the data as if real and imaginary parts are in separate halves of the buffer. They are NOT. The data is interleaved as Complex<float> pairs.

### performFrequencyOnlyForwardTransform (PREFERRED)

```cpp
void performFrequencyOnlyForwardTransform (float* inputOutputData,
                                           bool onlyCalculateNonNegativeFrequencies = false) const noexcept;
```

This is a convenience method that:
1. Calls `performRealOnlyForwardTransform`
2. Converts the interleaved complex output to magnitudes in-place using `std::abs(out[i])`
3. Zeros the remaining buffer

**Output layout:**
```
When onlyCalculateNonNegativeFrequencies = true:
  inputOutputData[0] = magnitude of bin 0 (DC)
  inputOutputData[1] = magnitude of bin 1
  ...
  inputOutputData[size/2] = magnitude of bin size/2 (Nyquist)
  inputOutputData[size/2+1 .. 2*size-1] = 0.0f (zeroed)
```

**Verified from source (juce_FFT.cpp lines 991-1005):**
```cpp
void FFT::performFrequencyOnlyForwardTransform (float* inputOutputData, bool ignoreNegativeFreqs) const noexcept
{
    performRealOnlyForwardTransform (inputOutputData, ignoreNegativeFreqs);
    auto* out = reinterpret_cast<Complex<float>*> (inputOutputData);
    const auto limit = ignoreNegativeFreqs ? (size / 2) + 1 : size;
    for (int i = 0; i < limit; ++i)
        inputOutputData[i] = std::abs (out[i]);
    zeromem (inputOutputData + limit, static_cast<size_t> (size * 2 - limit) * sizeof (float));
}
```

**Recommendation for MFCC pipeline:** Use `performFrequencyOnlyForwardTransform(data, true)` to get magnitude spectrum directly. This avoids manual complex-to-magnitude conversion and is less error-prone. The output gives exactly `(FFT_SIZE / 2) + 1` magnitude values in `data[0..FFT_SIZE/2]`.

### FFT Engine Priority on macOS

On macOS, JUCE uses the Apple vDSP framework (priority 5) over the fallback engine (priority -1). This is hardware-accelerated and significantly faster. The data layout is the same regardless of engine.

---

## 2. juce::dsp::WindowingFunction - Verified API Details

**Source:** `/Users/taylorbrook/JUCE/modules/juce_dsp/frequency/juce_Windowing.h` and `juce_Windowing.cpp`
**Confidence:** HIGH (read directly from JUCE 8.0.4 source)

### Constructor

```cpp
WindowingFunction (size_t size, WindowingMethod type, bool normalise = true, FloatType beta = 0);
```

**CRITICAL: The `normalise` parameter.**

When `normalise = true` (DEFAULT), the window table is scaled so that its sum equals `size`:
```cpp
// From juce_Windowing.cpp lines 162-173
if (normalise)
{
    FloatType sum (0);
    for (size_t i = 0; i < size; ++i)
        sum += samples[i];
    auto factor = static_cast<FloatType> (size) / sum;
    FloatVectorOperations::multiply (samples, factor, static_cast<int> (size));
}
```

This means the DC component of a windowed signal has the same amplitude as the unwindowed signal. This is useful for spectrum analyzers.

**For MFCC extraction, use `normalise = false`.** The standard MFCC pipeline expects the raw Hann window coefficients (`0.5 - 0.5 * cos(2*pi*i/(N-1))`). Normalization would alter the relative magnitudes in the mel filterbank, producing non-standard MFCC values. If you later want to compare MFCCs with reference implementations (librosa, Essentia), use unnormalized windows.

### Correct Construction for MFCC

```cpp
static constexpr int FFT_SIZE = 2048;

// For MFCC analysis windowing - use normalise=false
juce::dsp::WindowingFunction<float> hannWindow {
    static_cast<size_t>(FFT_SIZE),
    juce::dsp::WindowingFunction<float>::hann,
    false  // DO NOT normalise for MFCC extraction
};
```

### multiplyWithWindowingTable

```cpp
void multiplyWithWindowingTable (FloatType* samples, size_t size) const noexcept;
```

This multiplies `samples[i] *= windowTable[i]` for `i` in `0..min(size, windowTable.size())-1`. Uses `FloatVectorOperations::multiply` for SIMD optimization.

**Usage:**
```cpp
// Apply window to audio frame BEFORE FFT
hannWindow.multiplyWithWindowingTable(frameBuffer.data(), static_cast<size_t>(FFT_SIZE));
```

### Hann Window Formula (verified from source)

```cpp
// juce_Windowing.cpp lines 83-89
case hann:
{
    for (size_t i = 0; i < size; ++i)
    {
        auto cos2 = ncos<FloatType> (2, i, size);
        samples[i] = static_cast<FloatType> (0.5 - 0.5 * cos2);
    }
}
```

Where `ncos(order, i, size)` = `cos(order * i * pi / (size - 1))`.

This is the standard Hann window: `w[i] = 0.5 * (1 - cos(2*pi*i / (N-1)))`.

---

## 3. Mel Filterbank Construction

**Confidence:** HIGH (standard algorithm, verified against librosa implementation)

### Complete Implementation

```cpp
static constexpr int FFT_SIZE = 2048;
static constexpr int NUM_MEL_FILTERS = 40;
static constexpr float MEL_LOW_HZ = 20.0f;    // Skip DC, start at 20 Hz

// Filter matrix: melFilterbank[filterIndex][fftBin]
// Using vector of vectors for clarity; in production use flat array
std::array<std::vector<float>, NUM_MEL_FILTERS> melFilterbank;

static float hzToMel(float hz) {
    return 2595.0f * std::log10(1.0f + hz / 700.0f);
}

static float melToHz(float mel) {
    return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
}

void buildMelFilterbank(double sampleRate)
{
    const int numBins = FFT_SIZE / 2 + 1;  // 1025 bins for 2048-point FFT
    const float nyquist = static_cast<float>(sampleRate) / 2.0f;

    const float melMin = hzToMel(MEL_LOW_HZ);
    const float melMax = hzToMel(nyquist);

    // NUM_MEL_FILTERS + 2 points: left edge, center of each filter, right edge
    std::vector<float> melPoints(NUM_MEL_FILTERS + 2);
    for (int i = 0; i < NUM_MEL_FILTERS + 2; ++i)
        melPoints[i] = melMin + static_cast<float>(i) * (melMax - melMin)
                        / static_cast<float>(NUM_MEL_FILTERS + 1);

    // Convert mel points back to Hz
    std::vector<float> hzPoints(NUM_MEL_FILTERS + 2);
    for (int i = 0; i < NUM_MEL_FILTERS + 2; ++i)
        hzPoints[i] = melToHz(melPoints[i]);

    // Convert Hz to FFT bin indices (float for interpolation)
    std::vector<float> binPoints(NUM_MEL_FILTERS + 2);
    for (int i = 0; i < NUM_MEL_FILTERS + 2; ++i)
        binPoints[i] = hzPoints[i] * static_cast<float>(FFT_SIZE)
                        / static_cast<float>(sampleRate);

    // Build triangular filters
    for (int m = 0; m < NUM_MEL_FILTERS; ++m)
    {
        melFilterbank[m].resize(numBins, 0.0f);

        float leftBin   = binPoints[m];
        float centerBin = binPoints[m + 1];
        float rightBin  = binPoints[m + 2];

        for (int k = 0; k < numBins; ++k)
        {
            float binF = static_cast<float>(k);

            if (binF >= leftBin && binF <= centerBin && centerBin != leftBin)
                melFilterbank[m][k] = (binF - leftBin) / (centerBin - leftBin);
            else if (binF > centerBin && binF <= rightBin && rightBin != centerBin)
                melFilterbank[m][k] = (rightBin - binF) / (rightBin - centerBin);
            else
                melFilterbank[m][k] = 0.0f;
        }
    }
}
```

### Key Details

- **20 Hz lower bound** (not 0 Hz): The architecture doc shows `hzToMel(0.0f)` as the lower bound. Using 0 Hz wastes the first few mel filters on frequencies below audibility. 20 Hz is the standard lower bound used by librosa and Essentia. However, 0 Hz also works and is simpler -- the bottom filters just capture very little energy.

- **Nyquist upper bound**: Use `sampleRate / 2.0` (not hardcoded 20000 Hz). This adapts to the actual sample rate. At 44100 Hz, Nyquist = 22050 Hz.

- **Number of bins**: `FFT_SIZE / 2 + 1` = 1025 for a 2048-point FFT. This matches the output of `performFrequencyOnlyForwardTransform(data, true)`.

- **Triangular filter shape**: Rising slope from left edge to center, falling slope from center to right edge. Adjacent filters overlap at their edges (the right edge of filter m is the center of filter m+1).

### Performance Optimization: Sparse Storage

Most mel filter weights are zero (each filter only spans a few bins). For 10,000+ grains, storing only non-zero weights is worthwhile:

```cpp
struct MelFilter {
    int startBin;         // First non-zero bin
    int endBin;           // Last non-zero bin (exclusive)
    std::vector<float> weights;  // Only non-zero weights
};

std::array<MelFilter, NUM_MEL_FILTERS> melFilters;
```

This reduces the inner loop from 1025 iterations to ~20-60 iterations per filter. For 40 filters, that is ~1000-2400 multiply-adds instead of 41,000. At 10,000 grains this saves significant time.

---

## 4. DCT (Discrete Cosine Transform) Type-II

**Confidence:** HIGH (standard mathematical formula, trivial to implement)

### Pre-computed DCT Matrix

The Type-II DCT is defined as:

```
C[k] = sum_{n=0}^{N-1} x[n] * cos(pi * k * (n + 0.5) / N)
```

Where k is the output coefficient index (0..12 for 13 MFCCs) and N is the number of mel bands (40).

```cpp
static constexpr int NUM_COEFFICIENTS = 13;
static constexpr int NUM_MEL_FILTERS = 40;

// DCT matrix: dctMatrix[coefficient][melBand]
std::array<std::array<float, NUM_MEL_FILTERS>, NUM_COEFFICIENTS> dctMatrix;

void buildDCTMatrix()
{
    for (int c = 0; c < NUM_COEFFICIENTS; ++c)
    {
        for (int m = 0; m < NUM_MEL_FILTERS; ++m)
        {
            dctMatrix[c][m] = std::cos(
                juce::MathConstants<float>::pi
                * static_cast<float>(c)
                * (static_cast<float>(m) + 0.5f)
                / static_cast<float>(NUM_MEL_FILTERS)
            );
        }
    }
}
```

### Applying DCT

```cpp
std::array<float, NUM_COEFFICIENTS> mfccs;

for (int c = 0; c < NUM_COEFFICIENTS; ++c)
{
    float sum = 0.0f;
    for (int m = 0; m < NUM_MEL_FILTERS; ++m)
        sum += dctMatrix[c][m] * logMelEnergies[m];
    mfccs[c] = sum;
}
```

### Notes on DCT Normalization

Some implementations (e.g., librosa) apply an orthogonal normalization factor:
- `scale[0] = sqrt(1.0 / N)` for k=0
- `scale[k] = sqrt(2.0 / N)` for k>0

This is NOT required for our use case. Since we z-score normalize all descriptors after extraction, the DCT scaling factor is absorbed into the normalization. Omitting it simplifies the code without affecting KD-tree search quality.

### Matrix Size

The DCT matrix is 13 x 40 = 520 floats = 2,080 bytes. Trivially small, computed once.

---

## 5. Pre-emphasis Filter

**Confidence:** HIGH (standard signal processing)

### Formula

```
y[n] = x[n] - alpha * x[n-1]
```

Where `alpha = 0.97` (standard value, boosts high frequencies to compensate for the natural spectral rolloff of speech/audio).

### Per-grain Application (RECOMMENDED)

Pre-emphasis should be applied **per grain, independently**. Each grain is an independent analysis unit.

```cpp
void applyPreEmphasis(float* samples, int numSamples, float alpha = 0.97f)
{
    // Process in reverse to avoid overwriting needed values
    // (or use a temporary buffer)
    float prev = samples[0];
    for (int i = numSamples - 1; i >= 1; --i)
        samples[i] = samples[i] - alpha * samples[i - 1];
    // samples[0] remains unchanged (no previous sample available)
}
```

**Why per-grain, not across the entire file:**

1. **Grains overlap by 50%.** If you apply pre-emphasis across the entire file first, overlapping grains would share the same pre-emphasized data -- this is fine. But grains at the very start would behave differently (no prior sample context).

2. **The standard approach** in MFCC literature (Davis & Mermelstein 1980, HTK Book) applies pre-emphasis per-frame. The first sample of each frame uses the last sample of the previous frame or zero.

3. **For our use case**, the difference is negligible. With 2048-sample frames, the boundary effect of one sample is irrelevant. The simpler per-grain approach (with `samples[0]` unchanged or using the sample before the grain start) is correct.

**Recommended approach:**

```cpp
// When extracting from corpus buffer, use the sample before grain start
// for the first pre-emphasis value (if available)
void extractWithPreEmphasis(const juce::AudioBuffer<float>& corpus,
                            int grainStart, int grainLength,
                            float* output, float alpha = 0.97f)
{
    const float* src = corpus.getReadPointer(0);

    // First sample: use previous sample if available
    float prev = (grainStart > 0) ? src[grainStart - 1] : 0.0f;
    output[0] = src[grainStart] - alpha * prev;

    for (int i = 1; i < grainLength; ++i)
        output[i] = src[grainStart + i] - alpha * src[grainStart + i - 1];
}
```

---

## 6. Complete MFCC Extraction Pipeline

**Confidence:** HIGH (all components verified against source code)

### Corrected Implementation

This corrects the errors in ARCHITECTURE.md and uses the preferred JUCE APIs:

```cpp
class MFCCExtractor
{
public:
    static constexpr int FFT_ORDER = 11;
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;  // 2048
    static constexpr int NUM_BINS = FFT_SIZE / 2 + 1; // 1025
    static constexpr int NUM_MEL_FILTERS = 40;
    static constexpr int NUM_COEFFICIENTS = 13;

    MFCCExtractor()
        : fft(FFT_ORDER),
          hannWindow(static_cast<size_t>(FFT_SIZE),
                     juce::dsp::WindowingFunction<float>::hann,
                     false)  // normalise = false for MFCC
    {}

    void prepare(double sampleRate)
    {
        buildMelFilterbank(sampleRate);
        buildDCTMatrix();
    }

    // Extract 13 MFCCs from an audio grain
    // grainSamples: pointer into corpus buffer at grain start
    // grainLength: number of samples in grain (may be > FFT_SIZE)
    void extract(const float* grainSamples, int grainLength,
                 std::array<float, NUM_COEFFICIENTS>& outMFCCs)
    {
        // If grain is longer than FFT_SIZE, analyze the center portion
        // If grain is shorter than FFT_SIZE, zero-pad
        int offset = std::max(0, (grainLength - FFT_SIZE) / 2);
        int copyLen = std::min(grainLength, FFT_SIZE);

        // 1. Copy and pre-emphasize into work buffer
        std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
        fftBuffer[0] = grainSamples[offset];
        for (int i = 1; i < copyLen; ++i)
            fftBuffer[i] = grainSamples[offset + i]
                         - 0.97f * grainSamples[offset + i - 1];

        // 2. Apply Hann window
        hannWindow.multiplyWithWindowingTable(fftBuffer.data(),
                                              static_cast<size_t>(FFT_SIZE));

        // 3. FFT -> magnitude spectrum (in-place)
        fft.performFrequencyOnlyForwardTransform(fftBuffer.data(), true);
        // fftBuffer[0..NUM_BINS-1] now contains magnitudes

        // 4. Apply mel filterbank
        std::array<float, NUM_MEL_FILTERS> melEnergies;
        for (int m = 0; m < NUM_MEL_FILTERS; ++m)
        {
            float energy = 0.0f;
            for (int k = 0; k < NUM_BINS; ++k)
                energy += fftBuffer[k] * melFilterbank[m][k];
            melEnergies[m] = std::log10(std::max(energy, 1e-10f));
        }

        // 5. DCT
        for (int c = 0; c < NUM_COEFFICIENTS; ++c)
        {
            float sum = 0.0f;
            for (int m = 0; m < NUM_MEL_FILTERS; ++m)
                sum += dctMatrix[c][m] * melEnergies[m];
            outMFCCs[c] = sum;
        }
    }

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> hannWindow;

    // Work buffer: must be 2 * FFT_SIZE
    std::array<float, FFT_SIZE * 2> fftBuffer {};

    // Pre-computed matrices
    std::array<std::array<float, NUM_BINS>, NUM_MEL_FILTERS> melFilterbank {};
    std::array<std::array<float, NUM_MEL_FILTERS>, NUM_COEFFICIENTS> dctMatrix {};

    void buildMelFilterbank(double sampleRate) { /* ... as in Section 3 ... */ }
    void buildDCTMatrix()                       { /* ... as in Section 4 ... */ }
};
```

### Key Differences from ARCHITECTURE.md

| Issue | ARCHITECTURE.md | Corrected |
|-------|----------------|-----------|
| FFT magnitude extraction | `fftData[k]` for real, `fftData[FFT_SIZE-k]` for imag | Use `performFrequencyOnlyForwardTransform` -- magnitudes directly in `fftBuffer[0..NUM_BINS-1]` |
| FFT buffer size | `FFT_SIZE * 2` (correct) | Same, `FFT_SIZE * 2` |
| Window normalise | Not specified | Explicitly `false` |
| FFT order | `std::log2(FFT_SIZE)` (works but returns double) | Use integer constant `11` or `static_cast<int>(std::log2(FFT_SIZE))` |
| Mel filterbank lower Hz | 0 Hz | 20 Hz recommended (but 0 Hz also works) |

---

## 7. Spectral Descriptor Extraction

**Confidence:** HIGH (standard formulas, extracted from same FFT magnitude data)

All spectral descriptors use the same magnitude spectrum output from `performFrequencyOnlyForwardTransform`. Extract them in a single pass after the FFT.

### Alternative: Use performRealOnlyForwardTransform for Both Magnitude and Power

If you need both magnitude spectrum (for MFCCs) and individual real/imag values (not typical), use `performRealOnlyForwardTransform` and extract magnitudes manually:

```cpp
fft.performRealOnlyForwardTransform(fftBuffer.data(), true);
auto* complexData = reinterpret_cast<std::complex<float>*>(fftBuffer.data());

for (int k = 0; k < NUM_BINS; ++k)
    magnitudes[k] = std::abs(complexData[k]);
```

For our pipeline, `performFrequencyOnlyForwardTransform` is simpler since we only need magnitudes.

### Spectral Centroid

Weighted mean of frequencies, weighted by magnitude. Indicates "brightness".

```cpp
float computeSpectralCentroid(const float* magnitudes, int numBins, double sampleRate)
{
    float numerator = 0.0f;
    float denominator = 0.0f;
    const float binWidth = static_cast<float>(sampleRate) / static_cast<float>(FFT_SIZE);

    for (int k = 0; k < numBins; ++k)
    {
        float freq = static_cast<float>(k) * binWidth;
        numerator += freq * magnitudes[k];
        denominator += magnitudes[k];
    }

    return (denominator > 1e-10f) ? (numerator / denominator) : 0.0f;
}
```

**Output:** Hz value. Typical range: 100-10000 Hz. Will be z-score normalized later.

### Spectral Flatness

Ratio of geometric mean to arithmetic mean of the magnitude spectrum. 0 = tonal (peaked), 1 = white noise (flat).

```cpp
float computeSpectralFlatness(const float* magnitudes, int numBins)
{
    // Compute in log domain for numerical stability
    float logSum = 0.0f;
    float linearSum = 0.0f;

    // Skip bin 0 (DC) -- it biases the result
    for (int k = 1; k < numBins; ++k)
    {
        logSum += std::log(std::max(magnitudes[k], 1e-20f));
        linearSum += magnitudes[k];
    }

    int N = numBins - 1;  // excluding DC
    float geometricMean = std::exp(logSum / static_cast<float>(N));
    float arithmeticMean = linearSum / static_cast<float>(N);

    return (arithmeticMean > 1e-10f)
        ? (geometricMean / arithmeticMean)
        : 0.0f;
}
```

**Note:** The geometric mean is computed via `exp(mean(log(x)))` to avoid numerical overflow/underflow with direct multiplication of 1024 values.

### Spectral Flux

Frame-to-frame spectral change. Requires storing the previous frame's magnitude spectrum.

```cpp
float computeSpectralFlux(const float* currentMag, const float* previousMag, int numBins)
{
    float flux = 0.0f;

    for (int k = 0; k < numBins; ++k)
    {
        float diff = currentMag[k] - previousMag[k];
        // Half-wave rectified: only count increases (onset-sensitive)
        if (diff > 0.0f)
            flux += diff * diff;
    }

    return std::sqrt(flux);
}
```

**For grain-based extraction:** Since grains are analyzed independently (not in sequence), spectral flux can be computed by:
1. Dividing the grain into 2 halves (or more sub-frames)
2. Computing FFT of each half
3. Taking the flux between consecutive sub-frames
4. Averaging the flux values

```cpp
float computeGrainSpectralFlux(const float* grainSamples, int grainLength,
                                juce::dsp::FFT& fft,
                                juce::dsp::WindowingFunction<float>& window)
{
    // Use half-grain sub-frames with 50% overlap
    const int subFrameSize = std::min(FFT_SIZE, grainLength);
    const int hop = subFrameSize / 2;

    std::array<float, FFT_SIZE * 2> prevBuffer {};
    std::array<float, FFT_SIZE * 2> currBuffer {};
    const int numBins = FFT_SIZE / 2 + 1;

    float totalFlux = 0.0f;
    int numFluxFrames = 0;

    // First sub-frame
    std::copy_n(grainSamples, std::min(subFrameSize, grainLength), prevBuffer.data());
    window.multiplyWithWindowingTable(prevBuffer.data(), FFT_SIZE);
    fft.performFrequencyOnlyForwardTransform(prevBuffer.data(), true);

    // Subsequent sub-frames
    for (int offset = hop; offset + subFrameSize <= grainLength; offset += hop)
    {
        std::fill(currBuffer.begin(), currBuffer.end(), 0.0f);
        std::copy_n(grainSamples + offset, subFrameSize, currBuffer.data());
        window.multiplyWithWindowingTable(currBuffer.data(), FFT_SIZE);
        fft.performFrequencyOnlyForwardTransform(currBuffer.data(), true);

        totalFlux += computeSpectralFlux(currBuffer.data(), prevBuffer.data(), numBins);
        ++numFluxFrames;

        std::copy(currBuffer.begin(), currBuffer.end(), prevBuffer.begin());
    }

    return (numFluxFrames > 0) ? (totalFlux / static_cast<float>(numFluxFrames)) : 0.0f;
}
```

**Alternative (simpler):** For short grains (50ms = 2205 samples at 44.1kHz, just barely larger than one FFT frame), use a single FFT of the grain and set spectral flux = 0. Only compute flux for grains longer than 2x FFT_SIZE. This is a pragmatic simplification since 50ms grains are essentially stationary.

### Spectral Rolloff

Frequency below which `rolloffPercent`% of the total spectral energy is concentrated.

```cpp
float computeSpectralRolloff(const float* magnitudes, int numBins,
                              double sampleRate, float rolloffPercent = 0.85f)
{
    float totalEnergy = 0.0f;
    for (int k = 0; k < numBins; ++k)
        totalEnergy += magnitudes[k] * magnitudes[k];  // power spectrum

    float threshold = rolloffPercent * totalEnergy;
    float cumulativeEnergy = 0.0f;
    const float binWidth = static_cast<float>(sampleRate) / static_cast<float>(FFT_SIZE);

    for (int k = 0; k < numBins; ++k)
    {
        cumulativeEnergy += magnitudes[k] * magnitudes[k];
        if (cumulativeEnergy >= threshold)
            return static_cast<float>(k) * binWidth;
    }

    return static_cast<float>(numBins - 1) * binWidth;
}
```

**Rolloff threshold:** 85% is standard per CONTEXT.md. Some implementations use 95%.

### RMS Energy

```cpp
float computeRMS(const float* audioFrame, int numSamples)
{
    float sumSquares = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        sumSquares += audioFrame[i] * audioFrame[i];
    return std::sqrt(sumSquares / static_cast<float>(numSamples));
}
```

**Note:** Compute this on the raw audio samples BEFORE pre-emphasis and windowing. Pre-emphasis and windowing are for spectral analysis; RMS should reflect the actual loudness of the grain.

### Zero-Crossing Rate

```cpp
float computeZCR(const float* audioFrame, int numSamples)
{
    int crossings = 0;
    for (int i = 1; i < numSamples; ++i)
    {
        if ((audioFrame[i - 1] >= 0.0f && audioFrame[i] < 0.0f) ||
            (audioFrame[i - 1] < 0.0f  && audioFrame[i] >= 0.0f))
            ++crossings;
    }
    return static_cast<float>(crossings) / static_cast<float>(numSamples - 1);
}
```

**Note:** Also compute on raw audio samples, not pre-emphasized or windowed data.

---

## 8. Z-Score Normalization

**Confidence:** HIGH (standard statistics)

### Two-Pass Computation

After extracting all 19D descriptors for all grains:

```cpp
struct NormalizationStats {
    std::array<float, 19> means {};
    std::array<float, 19> stddevs {};
};

NormalizationStats normalizeDescriptors(std::vector<GrainMetadata>& grains)
{
    NormalizationStats stats;
    const int numGrains = static_cast<int>(grains.size());

    if (numGrains == 0) return stats;

    // Pass 1: compute means
    for (const auto& grain : grains)
        for (int d = 0; d < 19; ++d)
            stats.means[d] += grain.descriptors[d];

    for (int d = 0; d < 19; ++d)
        stats.means[d] /= static_cast<float>(numGrains);

    // Pass 2: compute standard deviations
    for (const auto& grain : grains)
        for (int d = 0; d < 19; ++d)
        {
            float diff = grain.descriptors[d] - stats.means[d];
            stats.stddevs[d] += diff * diff;
        }

    for (int d = 0; d < 19; ++d)
    {
        stats.stddevs[d] = std::sqrt(stats.stddevs[d] / static_cast<float>(numGrains));
        // Prevent division by zero (constant dimensions)
        if (stats.stddevs[d] < 1e-10f)
            stats.stddevs[d] = 1.0f;
    }

    // Apply normalization
    for (auto& grain : grains)
        for (int d = 0; d < 19; ++d)
            grain.descriptors[d] = (grain.descriptors[d] - stats.means[d])
                                    / stats.stddevs[d];

    return stats;
}
```

### Storing Stats for Target Mapping

The `NormalizationStats` must be stored alongside the corpus for use during playback:

```cpp
struct Corpus {
    juce::AudioBuffer<float> audioBuffer;
    std::vector<GrainMetadata> grains;
    NormalizationStats normStats;     // <-- MUST persist
    // ... KD-tree, etc.
};
```

When building the target descriptor vector for KD-tree queries (from macro knobs), the target values must be in the same z-score space:

```cpp
// Macro knob value (0-1) mapped to z-score space
// Knob at 0.5 -> z-score 0.0 (mean)
// Knob at 0.0 -> z-score -2.0 (2 stddevs below mean)
// Knob at 1.0 -> z-score +2.0 (2 stddevs above mean)
float knobToZScore(float knobValue)
{
    return (knobValue * 4.0f) - 2.0f;  // Map [0,1] to [-2, +2]
}
```

The CONTEXT.md uses `(knobValue * 2.0 - 1.0)` which maps to [-1, +1]. Either range works; [-2, +2] covers more of the descriptor space. The choice depends on how "extreme" the knobs should feel.

---

## 9. Complete Descriptor Extraction Pipeline

**Confidence:** HIGH

### Extraction Order and Data Flow

```
For each grain in corpus:

  1. Read raw samples from corpus buffer [grainStart, grainStart + grainLength)
  2. Compute RMS Energy from raw samples
  3. Compute Zero-Crossing Rate from raw samples
  4. Copy samples to work buffer, apply pre-emphasis
  5. Apply Hann window
  6. performFrequencyOnlyForwardTransform -> magnitudes[0..1024]
  7. Compute Spectral Centroid from magnitudes
  8. Compute Spectral Flatness from magnitudes
  9. Compute Spectral Rolloff from magnitudes
  10. Compute Spectral Flux (from sub-frames, or set to 0 for short grains)
  11. Apply mel filterbank to magnitudes -> 40 mel energies
  12. Log compress mel energies
  13. Apply DCT -> 13 MFCCs

  Store: descriptors[0..12] = MFCCs
         descriptors[13] = Spectral Centroid
         descriptors[14] = Spectral Flatness
         descriptors[15] = Spectral Flux
         descriptors[16] = Spectral Rolloff
         descriptors[17] = RMS Energy
         descriptors[18] = Zero-Crossing Rate
```

### Reusing FFT Output

Steps 7-9 and steps 11-13 all use the same magnitude spectrum from step 6. Do NOT recompute the FFT. The magnitude data in `fftBuffer[0..1024]` is consumed by both the spectral descriptors and the mel filterbank.

However, note that `performFrequencyOnlyForwardTransform` **overwrites** the buffer in-place. If you need the magnitudes for multiple consumers, copy them first:

```cpp
// After FFT
fft.performFrequencyOnlyForwardTransform(fftBuffer.data(), true);

// Save magnitudes before they might be overwritten
std::array<float, NUM_BINS> magnitudes;
std::copy_n(fftBuffer.data(), NUM_BINS, magnitudes.data());

// Now use magnitudes for both spectral descriptors and mel filterbank
float centroid = computeSpectralCentroid(magnitudes.data(), NUM_BINS, sampleRate);
float flatness = computeSpectralFlatness(magnitudes.data(), NUM_BINS);
float rolloff  = computeSpectralRolloff(magnitudes.data(), NUM_BINS, sampleRate);
// ... mel filterbank also uses magnitudes
```

In practice, since `performFrequencyOnlyForwardTransform` stores magnitudes in `fftBuffer[0..NUM_BINS-1]` and zeros the rest, you can safely read from `fftBuffer` directly as long as you do not overwrite it.

---

## 10. Performance Estimates

**Confidence:** MEDIUM (estimates based on operation counts, not benchmarked)

### Per-Grain Extraction Cost

| Operation | Estimated Cost | Notes |
|-----------|---------------|-------|
| Pre-emphasis | ~2 us | 2048 multiplies + subtracts |
| Hann window multiply | ~1 us | SIMD via FloatVectorOperations |
| FFT (2048-point, vDSP) | ~10-30 us | Hardware accelerated on macOS |
| Magnitude spectrum | included | Done by performFrequencyOnlyForwardTransform |
| Spectral descriptors (4) | ~5 us | 4 x 1025-bin loops |
| Mel filterbank (40 filters) | ~20-50 us | 40 x 1025 multiply-accumulates (or ~40 x 40 with sparse) |
| Log compression (40 values) | ~1 us | 40 log10 calls |
| DCT (13 x 40) | ~2 us | 520 multiply-accumulates |
| RMS + ZCR | ~5 us | 2 x 2048-sample loops |
| **Total per grain** | **~50-100 us** | |

### Full Corpus Extraction (10,000 grains)

| Metric | Estimate |
|--------|----------|
| Grains | 10,000 |
| Per-grain time | ~75 us (average) |
| Total extraction | ~750 ms |
| Z-score normalization | ~5 ms |
| **Total pipeline** | **~0.75 seconds** |

This is well within the "file analysis completes within 10 seconds" NFR-2 requirement. The full pipeline (load + resample + segment + extract + normalize + PCA + KD-tree build) should complete in 2-4 seconds for a typical 5-minute audio file at 44.1kHz.

### Optimization Opportunities (if needed)

1. **Sparse mel filterbank**: Reduce mel filterbank from 40x1025 to 40x~40 operations (20x speedup for that stage)
2. **SIMD magnitude computation**: Use `juce::FloatVectorOperations` for mel filterbank dot products
3. **Batch FFT**: Some engines support batched FFT, but JUCE's API does not expose this
4. **Multithreading**: Split grains across multiple threads (each thread has its own MFCCExtractor instance with its own fftBuffer)

For 10,000 grains at 75 us each, single-threaded extraction takes ~0.75s. This is fast enough that optimization is unlikely to be needed.

---

## 11. Common Pitfalls

### Pitfall 1: Wrong FFT Data Layout

**What goes wrong:** Treating `performRealOnlyForwardTransform` output as separate real/imaginary halves instead of interleaved Complex<float>.
**Why it happens:** Some FFT libraries (e.g., FFTW's r2c) use split real/imaginary format. JUCE uses interleaved format.
**How to avoid:** Use `performFrequencyOnlyForwardTransform` which outputs magnitudes directly. If you need complex values, cast to `Complex<float>*`.
**Warning signs:** MFCCs that look wrong, spectral centroid returning nonsensical values.

### Pitfall 2: WindowingFunction normalise=true for Analysis

**What goes wrong:** Using the default `normalise=true` produces a window that sums to `size`, not the standard Hann window. MFCCs will have different scaling than reference implementations.
**Why it happens:** The default parameter is `true` because JUCE's primary windowing use case is spectrum analyzers where unity DC gain is desired.
**How to avoid:** Explicitly pass `false` for the normalise parameter when constructing the window for MFCC analysis.

### Pitfall 3: FFT Buffer Not Zeroed

**What goes wrong:** Garbage in the second half of the FFT buffer produces wrong results.
**Why it happens:** The buffer must be `2 * FFT_SIZE` floats. If only the first `FFT_SIZE` elements are populated, the second half contains uninitialized data.
**How to avoid:** Zero-initialize the entire `fftBuffer` before copying audio data into the first half.

### Pitfall 4: Spectral Flatness Overflow

**What goes wrong:** Computing geometric mean via direct multiplication of 1024 values causes underflow to zero.
**Why it happens:** `0.001^1024` is astronomically small.
**How to avoid:** Compute in log domain: `geometricMean = exp(mean(log(x)))`.

### Pitfall 5: Pre-emphasis on Windowed Data

**What goes wrong:** Applying pre-emphasis after windowing instead of before.
**Why it happens:** Pipeline ordering confusion.
**How to avoid:** The correct order is: pre-emphasis -> windowing -> FFT. Pre-emphasis is a time-domain filter that should be applied to the raw signal.

### Pitfall 6: Forgetting to Persist Normalization Stats

**What goes wrong:** Z-score normalization is applied to descriptors but the means/stddevs are discarded. When the macro knobs build a target descriptor, it is in un-normalized space and the KD-tree returns wrong grains.
**Why it happens:** The normalization pass feels like a one-time transform.
**How to avoid:** Store `NormalizationStats` in the Corpus struct. Use them to map knob values to z-score space.

### Pitfall 7: RMS/ZCR on Windowed Data

**What goes wrong:** Computing RMS and ZCR on pre-emphasized or windowed data instead of raw samples.
**Why it happens:** It is tempting to reuse the work buffer after windowing.
**How to avoid:** Compute RMS and ZCR FIRST from raw grain samples, before any spectral processing.

### Pitfall 8: Spectral Flux for Short Grains

**What goes wrong:** Trying to compute spectral flux from a single FFT frame (need at least 2 frames for a difference).
**Why it happens:** 50ms grains at 44.1kHz = 2205 samples, barely enough for one 2048-point FFT.
**How to avoid:** For grains shorter than 2 * FFT_SIZE, set spectral flux to 0.0. The z-score normalization will handle this gracefully.

---

## 12. Architecture Decision: Single-Frame vs Multi-Frame Analysis

**For 50ms grains at 44.1kHz (2205 samples):**

The grain is barely larger than one FFT frame (2048 samples). Two approaches:

### Option A: Single-Frame Analysis (RECOMMENDED)

Analyze the entire grain as one FFT frame (zero-pad if grain < FFT_SIZE, use center portion if grain > FFT_SIZE). This is simpler and sufficient because:

- 50ms grains are approximately stationary (spectral content does not change significantly)
- Single FFT captures the overall timbral character
- MFCCs from a single frame are standard for short segments
- Spectral flux is set to 0 (or near-zero) for single-frame grains

### Option B: Multi-Frame Analysis with Averaging

Split grain into overlapping sub-frames, compute descriptors per sub-frame, average. This is more accurate for longer grains (>100ms) where spectral content evolves.

**Recommendation:** Use Option A for the initial implementation. If grain sizes up to 500ms are used (22050 samples), the extractor can optionally average multiple frames. But for the default 50ms, single-frame is correct.

---

## Sources

### Primary (HIGH confidence)
- `/Users/taylorbrook/JUCE/modules/juce_dsp/frequency/juce_FFT.h` -- FFT class declaration, buffer requirements, API documentation
- `/Users/taylorbrook/JUCE/modules/juce_dsp/frequency/juce_FFT.cpp` -- FFT implementation, all engine variants (vDSP, FFTW, Fallback), `performFrequencyOnlyForwardTransform` implementation
- `/Users/taylorbrook/JUCE/modules/juce_dsp/frequency/juce_Windowing.h` -- WindowingFunction API, enum values
- `/Users/taylorbrook/JUCE/modules/juce_dsp/frequency/juce_Windowing.cpp` -- Window computation, normalise behavior, `multiplyWithWindowingTable` implementation

### Secondary (MEDIUM-HIGH confidence)
- `research/concatenative-synthesis-comprehensive.md` -- MFCC pipeline pseudocode, descriptor set recommendations
- `.planning/research/ARCHITECTURE.md` -- System architecture (NOTE: contains FFT data layout error)
- `.planning/stages/2-dsp/CONTEXT.md` -- Locked decisions for Stage 2

### Reference (standard algorithms)
- Davis, S.B. & Mermelstein, P. (1980). "Comparison of Parametric Representations for Monosyllabic Word Recognition in Continuously Spoken Sentences." IEEE TASSP.
- HTK Book (Cambridge University) -- MFCC extraction pipeline reference
- librosa documentation -- mel filterbank and DCT conventions
