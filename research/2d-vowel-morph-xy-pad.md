---
title: "2D Vowel Morph XY Pad"
created: 2026-04-04
juce_version: "8.0.4"
summary: "Technical reference for mapping 5 cardinal vowels onto a 2D XY pad with real-time formant interpolation. Covers vowel space geometry, interpolation methods (barycentric, IDW, Shepard, RBF), log-frequency morphing, smoothing, and JUCE implementation."
domain: dsp
type: research
keywords:
  - formant-synthesis
  - vowel-morphing
  - xy-pad
  - 2d-interpolation
  - barycentric
  - shepard
  - inverse-distance-weighting
  - juce-dsp
stages: [0, 1, 2]
agents: [dsp, research]
plugin: O-Formant
---

# 2D Vowel Morph XY Pad

**Technical Reference for O-Formant Plugin**

**Created:** April 2026
**Companion to:** `research/vocal-formant-synthesis.md` (1D linear morph, formant filter banks, KLATT)

---

## 1. Vowel Space Geometry

### 1.1 The Acoustic Vowel Space

The IPA vowel quadrilateral maps vowels along two perceptual axes derived from the first two formant frequencies:

- **F1 (vertical axis):** Correlates with tongue height / jaw openness. Low F1 = close/high vowel, high F1 = open/low vowel.
- **F2 (horizontal axis):** Correlates with tongue frontness/backness. High F2 = front vowel, low F2 = back vowel.

The traditional convention in phonetics plots F1 on the Y axis (inverted -- low values at top) and F2 on the X axis (inverted -- high values at left). For a synthesizer XY pad, we remap to a more intuitive orientation.

### 1.2 Cardinal Vowel Formant Frequencies (Male Speaker)

Canonical values from Peterson & Barney (1952), Hillenbrand et al. (1995), and Wikipedia's averaged reference:

| Vowel | IPA | F1 (Hz) | F2 (Hz) | F3 (Hz) | F4 (Hz) | F5 (Hz) |
|-------|-----|---------|---------|---------|---------|---------|
| A     | /a/ | 730     | 1090    | 2440    | 3400    | 4500    |
| E     | /e/ | 530     | 1840    | 2480    | 3400    | 4500    |
| I     | /i/ | 270     | 2290    | 3010    | 3400    | 4500    |
| O     | /o/ | 570     | 840     | 2410    | 3400    | 4500    |
| U     | /u/ | 300     | 870     | 2240    | 3400    | 4500    |

Cross-reference with Wikipedia averaged male values:

| Vowel | F1 (Hz) | F2 (Hz) |
|-------|---------|---------|
| /i/   | 240     | 2400    |
| /e/   | 390     | 2300    |
| /a/   | 850     | 1610    |
| /o/   | 360     | 640     |
| /u/   | 250     | 595     |

**Note:** The synthesis presets in `vocal-formant-synthesis.md` use values closer to the first table. The Wikipedia values trend lower for F1 of /a/ and higher for F2 of /i/. Either set works -- the key is internal consistency.

### 1.3 Formant Bandwidths

| Formant | Bandwidth (Hz) | Notes |
|---------|----------------|-------|
| F1      | 60-100         | Narrow, dominant resonance |
| F2      | 70-120         | Moderate |
| F3      | 100-150        | Broader |
| F4      | 150-250        | Ambient character |
| F5      | 200-300        | Subtle, mostly speaker identity |

### 1.4 Normalized XY Pad Coordinates

Map the 5 vowels onto a [0,1] x [0,1] XY pad. The mapping must preserve the acoustic relationships:

- **X axis:** Front (left, X=0) to Back (right, X=1) -- maps inversely to F2
- **Y axis:** Open/low (bottom, Y=0) to Close/high (top, Y=1) -- maps inversely to F1

Step 1: Establish F1/F2 ranges from the vowel data:
```
F1 range: 270 Hz (I) to 730 Hz (A)
F2 range: 840 Hz (O) to 2290 Hz (I)
```

Step 2: Normalize. For X: `x = 1.0 - (F2 - F2min) / (F2max - F2min)` (inverted: high F2 = front = left). For Y: `y = 1.0 - (F1 - F1min) / (F1max - F1min)` (inverted: low F1 = close = top).

Resulting normalized positions:

| Vowel | F1   | F2   | X (back→front) | Y (open→close) |
|-------|------|------|-----------------|-----------------|
| A     | 730  | 1090 | 0.83            | 0.00            |
| E     | 530  | 1840 | 0.31            | 0.43            |
| I     | 270  | 2290 | 0.00            | 1.00            |
| O     | 570  | 840  | 1.00            | 0.35            |
| U     | 300  | 870  | 0.98            | 0.93            |

**Visual layout on XY pad:**

```
Y=1.0  I(0.00,1.00)                          U(0.98,0.93)
       |                                       |
       |                                       |
       |         E(0.31,0.43)                  |
       |                                       |
       |                            O(1.00,0.35)
Y=0.0  |                   A(0.83,0.00)       |
       X=0.0                              X=1.0
       (front)                            (back)
```

This accurately represents the classic vowel triangle/quadrilateral: I top-left, U top-right, A bottom-center-right, E mid-left, O mid-right.

### 1.5 Alternative: Artistic Vowel Layout

For a more musically intuitive layout (vowels evenly distributed, pentagon shape), you could override acoustic accuracy:

```
         I (0.50, 1.00)
        / \
   E (0.10, 0.65)   U (0.90, 0.65)
       |                |
   A (0.20, 0.15)   O (0.80, 0.15)
```

**Recommendation:** Start with the acoustically accurate layout (Section 1.4). It produces the most natural-sounding transitions because the interpolation distances correlate with perceptual distances. Offer the pentagon layout as an alternative mode if desired.

---

## 2. Interpolation Methods

### 2.1 Method Comparison

| Method | Pros | Cons | Complexity | Best For |
|--------|------|------|------------|----------|
| **IDW (Inverse Distance Weighting)** | Simplest, no precomputation, handles any point layout | All 5 vowels always contribute (muddy at center), no guarantee of C1 continuity | O(N) per query | Prototyping |
| **Shepard (modified IDW)** | Tunable sharpness via power parameter, smooth | Same muddiness issue, can overshoot with wrong power | O(N) per query | Good default |
| **Barycentric (Delaunay)** | Only 3 nearest vowels contribute per triangle, C0 continuous, natural transitions | Requires triangulation precomputation, C0 only (gradient discontinuity at triangle edges) | O(1) per query after triangulation | Best perceptual results |
| **RBF (Radial Basis Function)** | Globally smooth (C∞ with Gaussian), mathematically elegant | Requires solving 5x5 linear system at init, can overshoot/ring, extrapolation unstable | O(N) per query, O(N³) precompute | Smooth but overkill |

### 2.2 Recommendation: Shepard Interpolation (Modified IDW)

**Shepard interpolation is the best balance of simplicity, quality, and real-time performance for this use case.**

Rationale:
- Only 5 data points -- RBF's sophistication is wasted, barycentric's triangulation is fragile with so few points
- Shepard's tunable power parameter lets you control the "sharpness" of vowel regions
- With power p=2, vowels near the cursor dominate naturally; at p=4, you get tighter vowel zones
- No precomputation, no triangulation edge artifacts
- Trivial to implement and tune

### 2.3 Shepard Interpolation Formula

Given N vowel positions `(x_i, y_i)` with associated formant parameter sets `P_i`, and cursor position `(x, y)`:

```
d_i = sqrt((x - x_i)² + (y - y_i)²)

If d_i < epsilon for any i:
    result = P_i  (snap to nearest vowel)
Else:
    w_i = 1 / d_i^p
    W = sum(w_i)
    result = sum(w_i * P_i) / W
```

Where `p` is the power parameter (typically 2.0 to 4.0).

**Key insight:** Higher `p` creates sharper vowel zones with more abrupt transitions. Lower `p` creates smoother blending but muddier center. **p = 2.5** is a good starting point for vowel morphing.

### 2.4 Complete Shepard Implementation (C++)

```cpp
struct VowelPreset
{
    float f[5];   // Formant frequencies F1-F5 (Hz)
    float bw[5];  // Formant bandwidths (Hz)
    float g[5];   // Formant gains (dB)
};

struct VowelPoint
{
    float x, y;           // Position on XY pad [0,1]
    VowelPreset preset;
};

class VowelMorpher2D
{
public:
    void setVowelPoints()
    {
        // Acoustically accurate positions (Section 1.4)
        vowels[0] = { 0.83f, 0.00f, // A
            {{ 730, 1090, 2440, 3400, 4500 }, { 80, 90, 120, 200, 250 }, { 0, -6, -12, -18, -24 }} };
        vowels[1] = { 0.31f, 0.43f, // E
            {{ 530, 1840, 2480, 3400, 4500 }, { 70, 90, 110, 200, 250 }, { 0, -8, -12, -18, -24 }} };
        vowels[2] = { 0.00f, 1.00f, // I
            {{ 270, 2290, 3010, 3400, 4500 }, { 60, 90, 100, 200, 250 }, { 0, -10, -15, -20, -26 }} };
        vowels[3] = { 1.00f, 0.35f, // O
            {{ 570,  840, 2410, 3400, 4500 }, { 70, 80, 110, 200, 250 }, { 0, -6, -12, -18, -24 }} };
        vowels[4] = { 0.98f, 0.93f, // U
            {{ 300,  870, 2240, 3400, 4500 }, { 70, 80, 100, 200, 250 }, { 0, -8, -14, -20, -26 }} };
    }

    VowelPreset interpolate(float x, float y) const
    {
        constexpr float epsilon = 1e-6f;
        constexpr int N = 5;

        float weights[N];
        float totalWeight = 0.0f;

        for (int i = 0; i < N; ++i)
        {
            float dx = x - vowels[i].x;
            float dy = y - vowels[i].y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < epsilon)
                return vowels[i].preset; // Snap to exact vowel

            weights[i] = 1.0f / std::pow(dist, power);
            totalWeight += weights[i];
        }

        // Normalize weights
        float invTotal = 1.0f / totalWeight;
        for (int i = 0; i < N; ++i)
            weights[i] *= invTotal;

        // Interpolate formant parameters
        VowelPreset result = {};
        for (int i = 0; i < N; ++i)
        {
            float w = weights[i];
            for (int f = 0; f < 5; ++f)
            {
                // Frequencies in log domain (see Section 3)
                result.f[f] += w * std::log(vowels[i].preset.f[f]);
                // Bandwidths and gains in linear domain
                result.bw[f] += w * vowels[i].preset.bw[f];
                result.g[f] += w * vowels[i].preset.g[f];
            }
        }

        // Convert frequencies back from log domain
        for (int f = 0; f < 5; ++f)
            result.f[f] = std::exp(result.f[f]);

        return result;
    }

    void setPower(float p) { power = p; }

private:
    VowelPoint vowels[5];
    float power = 2.5f; // Shepard power parameter
};
```

### 2.5 Barycentric Interpolation (Alternative)

If you want tighter vowel regions with only the 3 nearest vowels contributing at any point, use Delaunay triangulation + barycentric coordinates. With only 5 points this produces 4-6 triangles.

**Precomputed triangulation for the acoustic layout:**

```cpp
// Delaunay triangulation of the 5 vowel points
// Indices: A=0, E=1, I=2, O=3, U=4
// Triangles (precomputed -- verify with actual coordinates):
struct Triangle { int v0, v1, v2; };
const Triangle triangles[] = {
    { 2, 1, 4 },  // I, E, U  (top region)
    { 1, 0, 3 },  // E, A, O  (bottom-center)
    { 1, 3, 4 },  // E, O, U  (right-center)
    { 0, 1, 2 },  // A, E, I  (left -- may not exist depending on exact coords)
};
// NOTE: Compute exact triangulation at init with Bowyer-Watson or
// use a simple brute-force check for 5 points.

// Barycentric coordinate computation for point P in triangle (A, B, C):
bool barycentricCoords(float px, float py,
                       float ax, float ay, float bx, float by, float cx, float cy,
                       float& u, float& v, float& w)
{
    float v0x = cx - ax, v0y = cy - ay;
    float v1x = bx - ax, v1y = by - ay;
    float v2x = px - ax, v2y = py - ay;

    float dot00 = v0x * v0x + v0y * v0y;
    float dot01 = v0x * v1x + v0y * v1y;
    float dot02 = v0x * v2x + v0y * v2y;
    float dot11 = v1x * v1x + v1y * v1y;
    float dot12 = v1x * v2x + v1y * v2y;

    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    w = 1.0f - u - v;

    return (u >= 0.0f) && (v >= 0.0f) && (w >= 0.0f);
}

// Query: find containing triangle, compute barycentric weights, interpolate
VowelPreset barycentricInterpolate(float x, float y,
                                    const VowelPoint* vowels,
                                    const Triangle* tris, int numTris)
{
    for (int t = 0; t < numTris; ++t)
    {
        float u, v, w;
        auto& tri = tris[t];
        if (barycentricCoords(x, y,
                vowels[tri.v0].x, vowels[tri.v0].y,
                vowels[tri.v1].x, vowels[tri.v1].y,
                vowels[tri.v2].x, vowels[tri.v2].y,
                u, v, w))
        {
            // Interpolate using barycentric weights
            VowelPreset result = {};
            const float weights[3] = { w, v, u }; // w = weight for v0
            const int indices[3] = { tri.v0, tri.v1, tri.v2 };

            for (int i = 0; i < 3; ++i)
            {
                float wt = weights[i];
                for (int f = 0; f < 5; ++f)
                {
                    result.f[f] += wt * std::log(vowels[indices[i]].preset.f[f]);
                    result.bw[f] += wt * vowels[indices[i]].preset.bw[f];
                    result.g[f] += wt * vowels[indices[i]].preset.g[f];
                }
            }
            for (int f = 0; f < 5; ++f)
                result.f[f] = std::exp(result.f[f]);

            return result;
        }
    }

    // Fallback: point outside convex hull -- use nearest triangle edge projection
    // or fall back to Shepard interpolation
    return shepardFallback(x, y, vowels);
}
```

**Barycentric pros/cons vs Shepard for this use case:**
- Pro: Only 3 vowels contribute at any point -- cleaner, less muddy
- Pro: Moving along a triangle edge produces perfectly linear blend between 2 vowels
- Con: Gradient discontinuity at triangle edges (audible as slight "bump" during smooth cursor sweeps)
- Con: Need fallback for points outside the convex hull (corners of the XY pad)
- Con: More code complexity for marginal perceptual benefit with only 5 points

### 2.6 RBF Interpolation (Reference Only)

For completeness, RBF with Gaussian basis:

```cpp
// Setup (call once):
// Solve 5x5 system: Phi * W = F  for each formant parameter
// Phi[i][j] = exp(-0.5 * dist(vowel_i, vowel_j)^2 / r0^2)
// W = Phi^(-1) * F

// Query:
// result = sum_i( w_i * exp(-0.5 * dist(query, vowel_i)^2 / r0^2) )

// r0 = scale parameter, ~0.3 to 0.5 for [0,1] domain
```

**Not recommended** for this case: the 5x5 system can become ill-conditioned, Gaussian RBF can overshoot between vowels producing non-physical formant values, and the perceptual improvement over Shepard is negligible with only 5 data points.

---

## 3. Log-Frequency vs Linear Interpolation

### 3.1 Why Log Domain for Frequencies

Human pitch perception is logarithmic. An octave is always a 2:1 frequency ratio regardless of absolute frequency. Interpolating formant frequencies in the linear domain produces perceptually non-uniform morphs:

```
Linear midpoint of 300 Hz and 1200 Hz = 750 Hz
Log midpoint of 300 Hz and 1200 Hz = exp((ln(300) + ln(1200)) / 2) = 600 Hz
```

The log midpoint (600 Hz) is the geometric mean -- perceptually centered between the two frequencies. The linear midpoint (750 Hz) sounds biased toward the higher frequency.

### 3.2 How It Interacts with 2D Interpolation

The log transform applies **inside** whatever 2D interpolation method you use. The interpolation weights are computed in the XY pad's Euclidean space (linear), but the weighted sum of formant frequencies uses log values:

```cpp
// CORRECT: interpolate in log domain
for (int f = 0; f < numFormants; ++f)
    result.f[f] += weight * std::log(vowelPreset.f[f]);
// ... then exp() after summing

// WRONG: interpolate in linear domain
for (int f = 0; f < numFormants; ++f)
    result.f[f] += weight * vowelPreset.f[f];
```

### 3.3 What to Interpolate in Which Domain

| Parameter | Domain | Rationale |
|-----------|--------|-----------|
| Formant frequencies (F1-F5) | **Log** | Perceptually linear pitch spacing |
| Formant bandwidths (BW1-BW5) | **Linear** or **Log** | Linear is fine for synthesis; log if bandwidths span > 3:1 ratio |
| Formant gains (G1-G5 in dB) | **Linear** | Already in dB (log) scale |

### 3.4 Performance Note

`std::log` and `std::exp` are expensive per sample. Since formant parameters change at control rate (not audio rate), compute the interpolation once per block (or once per parameter update), not per sample.

---

## 4. Implementation Considerations

### 4.1 Edge and Corner Behavior

The cursor can go anywhere on the [0,1] x [0,1] pad, including regions outside the vowel convex hull (e.g., bottom-left corner where no vowel exists).

**Shepard handles this naturally:** all 5 vowels always contribute with distance-based weights. Moving into empty space creates a blend dominated by the nearest vowels. No special-casing needed.

**Barycentric requires fallback:** points outside the triangulation's convex hull have no containing triangle. Options:
1. Project onto nearest triangle edge (nearest-point-on-hull)
2. Extend the convex hull by adding virtual anchor points at pad corners
3. Fall back to Shepard for out-of-hull points (hybrid approach)

**Recommendation:** If using Shepard, no concern. If barycentric, add dummy anchor points at pad corners that blend the two nearest vowels.

### 4.2 Smoothing / Inertia for XY Position

Raw mouse/touch input on an XY pad produces discontinuous jumps (especially on click-to-position). This causes zipper noise in the formant filters.

**Two-layer smoothing approach:**

```cpp
class SmoothedXYPad
{
public:
    void prepare(double sampleRate, double blockSize)
    {
        // Smooth the XY position at control rate
        // Use ~20-50ms ramp time for responsive but smooth movement
        double smoothTimeMs = 30.0;
        double blocksPerSecond = sampleRate / blockSize;
        double alpha = 1.0 - std::exp(-1.0 / (smoothTimeMs * 0.001 * blocksPerSecond));
        smoothingAlpha = static_cast<float>(alpha);
    }

    void setTarget(float newX, float newY)
    {
        targetX = newX;
        targetY = newY;
    }

    // Call once per audio block
    void update()
    {
        currentX += smoothingAlpha * (targetX - currentX);
        currentY += smoothingAlpha * (targetY - currentY);
    }

    float getX() const { return currentX; }
    float getY() const { return currentY; }

private:
    float targetX = 0.5f, targetY = 0.5f;
    float currentX = 0.5f, currentY = 0.5f;
    float smoothingAlpha = 0.1f;
};
```

**Second layer:** Smooth the individual formant filter coefficients using `juce::SmoothedValue` with multiplicative smoothing for frequencies:

```cpp
// Per-formant smoothing (audio rate)
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedFreq[5];
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedBW[5];
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGain[5];

void prepare(double sampleRate)
{
    for (int i = 0; i < 5; ++i)
    {
        smoothedFreq[i].reset(sampleRate, 0.02);  // 20ms ramp
        smoothedBW[i].reset(sampleRate, 0.02);
        smoothedGain[i].reset(sampleRate, 0.02);
    }
}

void updateFormants(const VowelPreset& target)
{
    for (int i = 0; i < 5; ++i)
    {
        smoothedFreq[i].setTargetValue(target.f[i]);
        smoothedBW[i].setTargetValue(target.bw[i]);
        smoothedGain[i].setTargetValue(target.g[i]);
    }
}
```

**Use multiplicative smoothing for frequencies** -- this ensures the transition is perceptually linear (constant ratio change per sample) rather than linearly stepping through Hz values.

### 4.3 Independent Interpolation of F, BW, and G

Yes, interpolate all three independently. They are perceptually independent parameters:

- **Frequencies** define which vowel you hear
- **Bandwidths** define resonance sharpness (nasal vs oral character)
- **Gains** define spectral balance (brightness, formant prominence)

The existing `morphVowels()` in `vocal-formant-synthesis.md` already does this correctly for 1D. The 2D version simply replaces the linear blend factor `t` with the Shepard/barycentric weights.

### 4.4 How Many Formants: 3 vs 5

| Formants | Quality | Cost | Use Case |
|----------|---------|------|----------|
| 3 (F1-F3) | Recognizable vowels, slightly synthetic | Low | Lightweight, character effect |
| 5 (F1-F5) | Natural-sounding, fuller spectrum | Moderate | Realistic vocal synthesis |

**Recommendation:** Interpolate all 5 formants. The cost difference is negligible (5 biquad filters vs 3), and F4/F5 add presence and naturalness. F4 and F5 are relatively constant across vowels (~3400 Hz and ~4500 Hz) so they barely change during morphing, but their presence fills the spectrum.

If CPU is tight, F4 and F5 can use fixed values (no interpolation needed since they're nearly identical across all 5 vowels).

### 4.5 Control Rate Architecture

```
Audio Thread (per block):
  1. smoothedXY.update()                    // ~control rate
  2. VowelPreset target = morpher.interpolate(smoothedXY.getX(), smoothedXY.getY())
  3. updateFormants(target)                  // set SmoothedValue targets
  4. Per-sample loop:
     a. Read smoothedFreq[i].getNextValue() for each formant
     b. Recompute filter coefficients (or use coefficient smoothing)
     c. Process audio through filter bank
```

**Critical:** Do NOT recompute IIR filter coefficients per sample -- this is expensive. Instead:
- Recompute coefficients once per block using the block's target values, OR
- Recompute every N samples (e.g., every 32 samples) for smoother transitions, OR
- Use `juce::SmoothedValue` on the coefficients themselves (less common, works for simple biquads)

The most practical approach: recompute filter coefficients at the start of each block, and rely on the XY position smoothing + short block sizes (64-256 samples) to prevent audible discontinuities.

### 4.6 Shepard Power Parameter as User Control

Exposing the Shepard power `p` as a plugin parameter (e.g., "Focus" or "Sharpness") gives the user control over morph character:

| Power (p) | Behavior |
|-----------|----------|
| 1.0       | Very smooth, all vowels always present, washy |
| 2.0       | Natural blending, gentle transitions |
| 2.5       | Balanced (default) |
| 3.0-4.0   | Tight vowel zones, snappier transitions |
| 6.0+      | Near-discrete, almost snaps to nearest vowel |

---

## 5. Reference Implementations and Academic Work

### 5.1 Pink Trombone (Neil Thapen)

- **URL:** https://dood.al/pinktrombone/
- **Approach:** Physical model of vocal tract using digital waveguide synthesis
- **Tongue control:** 2D parameter space with `tongueIndex` (position along tract, 12-29) and `tongueDiameter` (opening, ~1.0-3.5)
- **Not formant-based** -- directly shapes the tract cross-section, formants emerge naturally
- **Vowel positions** in the tongue space:
  - /i/ (peat): index 27.4, diameter 0.21 (front, narrow)
  - /e/ (pet): index 20, diameter 1.00
  - /æ/ (pat): index 15, diameter 0.60
  - /ɑ/ (part): index 13, diameter 0.27
  - /ɒ/ (pot): index 12, diameter 0.00 (back, narrow)
  - /ʌ/ (putt): index 18.1, diameter 0.37
- **Relevance:** Demonstrates that 2D tongue control naturally maps to vowel space. The index→backness and diameter→openness mapping parallels F2→X and F1→Y.

### 5.2 PaulBatchelor/voc (C Library)

- **URL:** https://github.com/PaulBatchelor/voc
- **Port of Pink Trombone** to C using literate programming
- **License:** MIT / public domain
- Direct tract model, not formant interpolation
- Tongue `index` and `diameter` parameters exposed for real-time control

### 5.3 Homunculus (JUCE Formant Synth)

- **URL:** https://github.com/tmroyal/homunculus
- **License:** GPL3
- JUCE-based formant synthesizer with morph slider
- 1D morphing only (slider between presets), not 2D XY pad
- Useful reference for JUCE formant filter implementation

### 5.4 GROWL (Formant Filter)

- **URL:** https://github.com/PaulBatchelor/GROWL
- Formant filter inspired by the HOWL iOS app
- Simple formant filter implementation, useful for filter design reference

### 5.5 Interactive Vowel Space

- **URL:** https://www.yacavone.net/vowel-space/
- Web-based interactive vowel space visualization with audio
- Demonstrates F1/F2 mapping to cursor position

### 5.6 Academic References

- **Peterson & Barney (1952):** "Control methods used in a study of the vowels." JASA 24(2), 175-184. The canonical formant frequency dataset.
- **Hillenbrand et al. (1995):** "Acoustic characteristics of American English vowels." JASA 97(5), 3099-3111. Updated measurements, freely available dataset.
- **Klatt (1980):** "Software for a cascade/parallel formant synthesizer." JASA 67(3), 971-995. The gold standard parametric speech synthesizer.
- **Shepard (1968):** "A two-dimensional interpolation function for irregularly-spaced data." Proceedings of the 23rd ACM national conference. The original IDW method.

---

## 6. Complete Integration Pseudocode

```cpp
class FormantXYMorpher
{
public:
    void prepare(double sampleRate, int blockSize)
    {
        morpher.setVowelPoints();
        xyPad.prepare(sampleRate, blockSize);

        for (int i = 0; i < 5; ++i)
        {
            smoothedFreq[i].reset(sampleRate, 0.020); // 20ms
            smoothedBW[i].reset(sampleRate, 0.020);
            smoothedGain[i].reset(sampleRate, 0.020);
        }
    }

    void setXY(float x, float y)
    {
        xyPad.setTarget(x, y);
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        // 1. Smooth XY position
        xyPad.update();

        // 2. Interpolate vowel at current position
        auto target = morpher.interpolate(xyPad.getX(), xyPad.getY());

        // 3. Set smoothed targets
        for (int i = 0; i < 5; ++i)
        {
            smoothedFreq[i].setTargetValue(target.f[i]);
            smoothedBW[i].setTargetValue(target.bw[i]);
            smoothedGain[i].setTargetValue(target.g[i]);
        }

        // 4. Process audio with per-sample coefficient updates
        auto numSamples = buffer.getNumSamples();
        constexpr int updateInterval = 32; // Recompute coefficients every 32 samples

        for (int startSample = 0; startSample < numSamples; startSample += updateInterval)
        {
            int samplesToProcess = juce::jmin(updateInterval, numSamples - startSample);

            // Skip smoothed values forward
            float freqs[5], bws[5], gains[5];
            for (int i = 0; i < 5; ++i)
            {
                freqs[i] = smoothedFreq[i].skip(samplesToProcess);
                bws[i] = smoothedBW[i].skip(samplesToProcess);
                gains[i] = smoothedGain[i].skip(samplesToProcess);
            }

            // Recompute filter coefficients
            updateFilterCoefficients(freqs, bws, gains);

            // Process sub-block through filter bank
            processSubBlock(buffer, startSample, samplesToProcess);
        }
    }

private:
    VowelMorpher2D morpher;
    SmoothedXYPad xyPad;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedFreq[5];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedBW[5];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGain[5];
    juce::IIRFilter formantFilters[5];

    void updateFilterCoefficients(float* freqs, float* bws, float* gains)
    {
        for (int i = 0; i < 5; ++i)
        {
            float Q = freqs[i] / bws[i];
            float linearGain = juce::Decibels::decibelsToGain(gains[i]);
            formantFilters[i].setCoefficients(
                juce::IIRCoefficients::makeBandPass(sampleRate, freqs[i], Q));
            // Apply gain scaling per formant
        }
    }

    void processSubBlock(juce::AudioBuffer<float>& buffer, int start, int count)
    {
        // Sum parallel formant filters (not cascade for vowel synthesis)
        for (int s = start; s < start + count; ++s)
        {
            float input = buffer.getSample(0, s);
            float output = 0.0f;
            for (int f = 0; f < 5; ++f)
                output += formantFilters[f].processSingleSampleRaw(input);
            buffer.setSample(0, s, output);
        }
    }

    double sampleRate = 44100.0;
};
```

---

## 7. Summary: Recommended Architecture

1. **Vowel positions:** Acoustically accurate F1/F2 normalized coordinates (Section 1.4)
2. **Interpolation:** Shepard with p=2.5 (Section 2.3-2.4) -- simplest, smoothest, no edge artifacts
3. **Frequency domain:** Log interpolation for formant frequencies, linear for BW and gains (Section 3)
4. **Smoothing:** Two layers -- exponential smoothing on XY position (30ms), multiplicative SmoothedValue on formant frequencies (20ms) (Section 4.2)
5. **Formants:** All 5, with F4/F5 optionally fixed at 3400/4500 Hz (Section 4.4)
6. **Coefficient updates:** Every 32 samples within the audio block (Section 4.5)
7. **Optional user control:** Shepard power parameter ("Focus") for morph sharpness (Section 4.6)

---

## Sources

- [Vowel diagram - Wikipedia](https://en.wikipedia.org/wiki/Vowel_diagram)
- [Formant - Wikipedia](https://en.wikipedia.org/wiki/Formant)
- [The vowel space](https://www.englishspeechservices.com/blog/the-vowel-space/)
- [Inverse distance weighting - Wikipedia](https://en.wikipedia.org/wiki/Inverse_distance_weighting)
- [RBF Interpolation in 2D (Burkardt)](https://people.math.sc.edu/Burkardt/cpp_src/rbf_interp_2d/rbf_interp_2d.html)
- [Shepard Interpolation in 2D (Burkardt)](https://people.math.sc.edu/Burkardt/cpp_src/shepard_interp_2d/shepard_interp_2d.html)
- [Pink Trombone (Neil Thapen)](https://dood.al/pinktrombone/)
- [PaulBatchelor/voc](https://github.com/PaulBatchelor/voc)
- [zakaton/Pink-Trombone](https://github.com/zakaton/Pink-Trombone)
- [Homunculus JUCE Formant Synth](https://github.com/tmroyal/homunculus)
- [KVR: Linear Interpolation between formant filter coefficients](https://www.kvraudio.com/forum/viewtopic.php?t=61648)
- [KVR: Linear Morphing between values in a formant filter](https://www.kvraudio.com/forum/viewtopic.php?t=492329)
- [Peterson & Barney (1952) Praat reference](https://www.fon.hum.uva.nl/praat/manual/Create_formant_table__Peterson___Barney_1952_.html)
- [Hillenbrand et al. (1995) data](https://www.ling.upenn.edu/courses/cogs501/Hillenbrand.html)
- [Static Measurements of Vowel Formant Frequencies - PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC6002811/)
- [JUCE SmoothedValue reference](https://docs.juce.com/master/classSmoothedValue.html)
- [Scattered Data Interpolation (SIGGRAPH 2014)](https://scribblethink.org/Courses/ScatteredInterpolation/scatteredinterpcoursenotes.pdf)
- [Interactive Vowel Space](https://www.yacavone.net/vowel-space/)
- [Barycentric Interpolation (CGAL)](https://cgal.geometryfactory.com/CGAL/doc/main/Barycentric_coordinates_2/index.html)
