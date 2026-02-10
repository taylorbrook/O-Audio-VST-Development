# O-Orbit Architecture Specification

> **Contract Status:** DRAFT
> **Generated:** 2026-02-09
> **JUCE Version:** 8.0.4
> **Plugin Type:** Effect (mono/stereo input → 2-24 channel output)

---

## Executive Summary

O-Orbit is a universal orbital spatializer implementing **VBAP (Vector Base Amplitude Panning)** for arbitrary speaker arrays. The plugin generates time-varying source positions (azimuth, elevation, distance) via LFO-based motion paths, applies distance modeling (attenuation + air absorption), and renders to speaker feeds using VBAP gain calculation.

**Core Innovation:** Single plugin supports stereo through 7.1.4 Atmos through fully custom speaker layouts with automatic downmix fallback.

**Technical Approach:** VBAP gain computation (lightweight), no ambisonics encoding, no HRTF/binaural, no room simulation. This is a *direct amplitude panning* system with distance-based filtering.

---

## 1. Core Components

### 1.1 Motion Engine (LFO-Based Position Generator)

**Purpose:** Generate source position (azimuth, elevation, distance) over time based on path shape and parameters.

**JUCE Classes:**
- `juce::dsp::Oscillator<float>` - LFO for orbital motion
- Custom `MotionPathGenerator` class - Path shape computation

**Algorithm:**
- **Orbital Path:** `azimuth(t) = centerAz + width * cos(2π * speed * t + phase)`, `elevation(t) = centerEl + elevRange * sin(2π * speed * t)`
- **Pendulum Path:** Single-axis sinusoidal swing
- **Linear Path:** Constant-velocity sweep with wrapping
- **Drift Path:** Perlin noise or low-pass filtered random walk
- **Custom Path:** User-drawn path via WebView canvas, stored as spline

**Tempo Sync:**
- Read host BPM via `AudioPlayHead::getPosition()->getBpm()`
- Convert tempo divisions (1/16, 1/8, 1/4, 1/2, 1, 2, 4 bars) to Hz
- Formula: `frequency_hz = (bpm / 60) * tempo_division_factor`

**Thread Safety:** Motion state updated per-block on audio thread (single producer). UI reads for visualization (atomic position snapshots).

**Module Dependencies:** None (custom DSP)

---

### 1.2 VBAP Renderer

**Purpose:** Convert source direction to per-speaker gains using Vector Base Amplitude Panning.

**JUCE Classes:**
- Custom `VBAPRenderer` class (no JUCE equivalent)

**Algorithm Overview:**

**2D VBAP (ear-level speakers):**
1. Sort speakers by azimuth angle
2. For each adjacent pair `(L1, L2)`, precompute inverse matrix: `M^-1 = [l1_x, l1_y; l2_x, l2_y]^-1`
3. At runtime: find active pair for source direction `p`, solve `g = p^T * M^-1`, normalize `g = g / ||g||`

**3D VBAP (speakers with elevation):**
1. Convert speaker positions to Cartesian unit vectors
2. Compute Delaunay triangulation on unit sphere (convex hull)
3. Precompute inverse matrices for each triangle: `M^-1 = [l1, l2, l3]^T^-1` (3x3)
4. At runtime: find active triangle, solve `g = p^T * M^-1`, normalize

**Delaunay Triangulation:**
- Use SAF library `findLsTriplets()` OR simple qhull-based convex hull
- Precompute once per layout change (not real-time)
- Store triangle indices + inverted matrices

**Gain Smoothing:**
- Per-speaker gain interpolation over block to prevent zipper noise
- Linear ramp from `previousGain[speaker]` to `currentGain[speaker]`

**Spread Control (Center Diverge):**
- When `centerDiverge > 0`, activate additional adjacent speakers with reduced gain
- Formula: `spreadGain[adjacent] = centerDiverge * (1 - angularDistance / maxDistance)`

**Implementation Options:**
1. **Use SAF (Spatial Audio Framework):** `saf_vbap` module provides production-ready VBAP
   - License: ISC (permissive, commercial-friendly)
   - Handles 2D/3D, triangulation, gain tables
   - Dependency: CBLAS/LAPACK (Apple Accelerate on macOS, Intel MKL or OpenBLAS on Windows)

2. **Implement from Scratch:**
   - Core VBAP: ~300-400 lines
   - Delaunay: use `qhull` library or naive convex hull
   - Simpler build but less optimized

**Recommendation:** Use SAF for VBAP. Triangulation and gain table generation are non-trivial, SAF is battle-tested.

**Module Dependencies (SAF approach):**
- SAF: `saf_vbap`, `saf_utilities`
- JUCE: None (pure math)

---

### 1.3 Distance Model

**Purpose:** Attenuate level and apply frequency-dependent absorption based on source distance.

**JUCE Classes:**
- `juce::dsp::IIR::Filter<float>` - 1-pole LPF for air absorption
- `juce::dsp::IIR::Coefficients<float>::makeLowPass()`

**Attenuation Curves:**

| Curve | Formula | dB/Doubling | Use Case |
|-------|---------|-------------|----------|
| Inverse Square | `gain = refDist / distance` | -6 dB | Physical (free-field) |
| Inverse | `gain = refDist / distance` (linear scale) | -6 dB | Same as inverse square in dB |
| Linear | `gain = 1 - (distance - minDist) / (maxDist - minDist)` | Variable | Perceptually smooth |

**Air Absorption:**
- High frequencies attenuate more with distance
- Approximate with 1-pole LPF where cutoff decreases with distance
- Formula: `cutoff_hz = 20000 / (1 + airAbsorption * (distance / 10.0))`
- At `distance=10m, airAbsorption=1.0`: cutoff ~10kHz

**Per-Sample Processing:**
```cpp
// Per source, per block:
float distGain = referenceDistance / std::max(distance, 0.1f);
sample *= distGain;

// Air absorption LPF:
lpf.setCoefficients(IIR::Coefficients<float>::makeLowPass(sampleRate, cutoffFromDistance));
sample = lpf.processSample(sample);
```

**Module Dependencies:**
- JUCE: `juce::juce_dsp` (for IIR filters)

---

### 1.4 Speaker Layout System

**Purpose:** Define, store, and manage speaker configurations (preset + custom).

**Data Structure:**
```cpp
struct Speaker {
    float azimuth;    // degrees, 0=front, 90=left, -90=right, 180=back
    float elevation;  // degrees, 0=horizon, 90=up, -90=down
    float distance;   // meters from center (for non-equidistant arrays)
    juce::String label; // e.g., "L", "R", "C", "LFE", "Ls", "Rs"
};

struct SpeakerLayout {
    juce::String name;
    std::vector<Speaker> speakers;
    bool is3D; // true if any speaker has elevation != 0
};
```

**Preset Layouts:**

| Layout | Channels | Speaker Positions (az, el in degrees) |
|--------|----------|--------------------------------------|
| Stereo | 2 | L(-30,0), R(30,0) |
| Quad | 4 | L(-45,0), R(45,0), Ls(-135,0), Rs(135,0) |
| 5.1 (ITU) | 6 | L(-30,0), R(30,0), C(0,0), LFE(0,0), Ls(-110,0), Rs(110,0) |
| 7.1 | 8 | 5.1 + Lss(-90,0), Rss(90,0) |
| 5.1.4 | 10 | 5.1 + Ltf(-45,45), Rtf(45,45), Ltr(-135,45), Rtr(135,45) |
| 7.1.4 | 12 | 7.1 + 4 height (same as 5.1.4 heights) |
| Hexaphonic | 6 | 6 speakers at 60° intervals on horizon |
| Octaphonic | 8 | 8 speakers at 45° intervals on horizon |

**Custom Layout:**
- User adds/removes speakers via WebView UI
- Drag to reposition (updates azimuth, elevation)
- Distance editable via text input
- Saved to plugin state as XML or JSON

**Persistence:**
- Store in `AudioProcessorValueTreeState` or custom `MemoryBlock`
- Format: `<Layout name="Custom1"><Speaker az="45" el="30" dist="1.0" label="FL"/></Layout>`

**Module Dependencies:**
- JUCE: `juce::ValueTree`, `juce::XmlElement`

---

### 1.5 Source Mode (Mono / L+R Split)

**Purpose:** Handle mono or stereo input as single or dual orbiting sources.

**Modes:**

| Mode | Description | Processing |
|------|-------------|------------|
| Mono | Sum L+R to mono, single orbit | `mono = (L + R) * 0.5` |
| L+R Split | Two independent orbits with phase offset | Orbit L at phase=0°, orbit R at phase=offset° |

**L/R Offset Parameter:**
- Range: 0-360 degrees
- Offsets the R channel's starting position on the orbital path
- Example: offset=180° places L and R on opposite sides of the orbit

**Implementation:**
```cpp
if (sourceMode == SourceMode::Mono) {
    float mono = (inputL + inputR) * 0.5f;
    processSource(mono, motionState.azimuth, motionState.elevation, motionState.distance);
} else { // L+R Split
    processSource(inputL, motionState.azimuth, motionState.elevation, motionState.distance);
    float offsetAz = wrapAngle(motionState.azimuth + lrOffsetDegrees);
    processSource(inputR, offsetAz, motionState.elevation, motionState.distance);
}
```

**Module Dependencies:** None (pure routing logic)

---

### 1.6 Auto-Downmix (Format Fallback)

**Purpose:** When DAW provides fewer channels than configured layout, fold down gracefully.

**Detection:**
```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    auto outputLayout = getBusesLayout().getMainOutputChannelSet();
    int availableChannels = outputLayout.size();
    int requiredChannels = currentSpeakerLayout.speakers.size();

    if (availableChannels < requiredChannels) {
        isDownmixActive = true;
        downmixToChannels = availableChannels;
        // Generate downmix matrix
        createDownmixMatrix(currentSpeakerLayout, availableChannels);
    }
}
```

**Downmix Strategies:**

| Target | Strategy |
|--------|----------|
| Stereo | Energy-preserving fold: sum speaker gains by azimuth to L/R using equal-power panning |
| 5.1 | Fold height speakers to ear-level equivalents |
| Arbitrary N | Select N nearest speakers, redistribute gains |

**Energy-Preserving Stereo Downmix:**
```cpp
// For each VBAP speaker feed:
float azRad = speaker.azimuth * M_PI / 180.0f;
float panAngle = (azRad + M_PI/2) / M_PI; // Normalize to [0, 1] for L→R
float gainL = std::cos(panAngle * M_PI / 2);
float gainR = std::sin(panAngle * M_PI / 2);
stereoL += speakerFeed * gainL;
stereoR += speakerFeed * gainR;
```

**UI Warning:**
- Display badge in WebView: "Layout: 7.1.4 → DAW: Stereo"
- Non-modal, unobtrusive visual indicator

**Module Dependencies:** None (custom mix matrix)

---

## 2. Processing Chain (Signal Flow)

```
Input Buffer (mono or stereo)
        ↓
[1. Source Mode Handling]
  - Mono: sum L+R
  - L+R Split: dual sources
        ↓
[2. Motion Engine - Per Block]
  - Update LFO phase
  - Compute azimuth(t), elevation(t), distance(t)
  - Handle tempo sync (read host BPM)
        ↓
[3. Distance Model - Per Sample]
  FOR EACH SOURCE:
    - Apply distance attenuation: gain = refDist / distance
    - Apply air absorption LPF (cutoff based on distance)
        ↓
[4. VBAP Renderer - Per Block]
  FOR EACH SOURCE:
    - Convert (azimuth, elevation) to unit vector
    - Find active speaker pair/triplet via VBAP
    - Compute speaker gains
    - Smooth gains (linear ramp from previous)
        ↓
[5. Mix]
  - Accumulate all source contributions per speaker
  - Apply wet/dry mix parameter
        ↓
[6. Auto-Downmix (if active)]
  - Apply downmix matrix to fold speakers to available channels
        ↓
Output Buffer (2-24 channels)
```

**Block vs. Sample-Rate Processing:**
- **Per-block:** Motion state update, VBAP gain calculation (expensive triangle search)
- **Per-sample:** Distance gain/LPF, speaker feed accumulation (cheap multiply-accumulate)

**Thread Boundaries:**
- **Audio Thread:** All DSP processing
- **UI Thread:** Parameter changes (via APVTS), speaker layout editing
- **Background Thread (optional):** VBAP triangulation when layout changes (non-real-time)

---

## 3. System Architecture

### 3.1 Multi-Channel Bus Configuration

**JUCE 8 BusesProperties Configuration:**

```cpp
MyPluginProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}

bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
    auto mainInput = layouts.getMainInputChannelSet();
    auto mainOutput = layouts.getMainOutputChannelSet();

    // Require input
    if (mainInput.isDisabled() || mainOutput.isDisabled())
        return false;

    // Accept mono or stereo input
    if (mainInput != juce::AudioChannelSet::mono() &&
        mainInput != juce::AudioChannelSet::stereo())
        return false;

    // Output: discrete channel counts 2-24
    int outCh = mainOutput.size();
    if (outCh < 2 || outCh > 24)
        return false;

    // Accept any discrete layout (JUCE handles channel set negotiation)
    return true;
}
```

**Key Insight:** O-Orbit does NOT use named JUCE layouts (e.g., `create7point1point4()`). It accepts any channel count 2-24 and treats channels as discrete speaker feeds based on the user's custom layout configuration.

**Runtime Layout Detection:**
```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    int outputChannels = getTotalNumOutputChannels();

    // Match current speaker layout to output channel count
    if (currentSpeakerLayout.speakers.size() != outputChannels) {
        // Auto-downmix or warn user
        handleChannelMismatch(outputChannels);
    }
}
```

**Critical Pattern (from juce8-critical-patterns.md):**
- Do NOT use `PLUGIN_CHANNEL_CONFIGURATIONS` in CMakeLists.txt
- Use `isBusesLayoutSupported()` for dynamic negotiation
- Bus configuration must be set in constructor `BusesProperties`, NOT in `prepareToPlay()`

---

### 3.2 Speaker Layout Persistence

**State Storage:**
- Current active layout name (preset or "Custom")
- If custom: full speaker list with positions
- Store in `AudioProcessorValueTreeState` under "speakerLayout" identifier

**XML Format:**
```xml
<SpeakerLayout name="Custom1" is3D="true">
    <Speaker az="0" el="0" dist="1.0" label="C"/>
    <Speaker az="-30" el="0" dist="1.0" label="L"/>
    <Speaker az="30" el="0" dist="1.0" label="R"/>
    <Speaker az="-110" el="0" dist="1.0" label="Ls"/>
    <Speaker az="110" el="0" dist="1.0" label="Rs"/>
    <Speaker az="-45" el="45" dist="1.0" label="Ltf"/>
    <!-- ... -->
</SpeakerLayout>
```

**Load/Save:**
- Export layout as `.oorbitspeakers` file (XML or JSON)
- Import from file
- Share layouts across projects

---

### 3.3 MIDI Input (None)

O-Orbit does NOT process MIDI. It is a pure audio effect.

**CMakeLists.txt:**
```cmake
NEEDS_MIDI_INPUT FALSE
IS_SYNTH FALSE
IS_MIDI_EFFECT FALSE
```

---

### 3.4 File I/O (Speaker Layout Import/Export)

**Required Operations:**
- Save custom layout to file
- Load layout from file

**Implementation:**
```cpp
void exportLayout(const juce::File& file) {
    auto xml = layoutToXML(currentSpeakerLayout);
    xml->writeTo(file);
}

SpeakerLayout importLayout(const juce::File& file) {
    auto xml = juce::parseXML(file);
    return xmlToLayout(xml.get());
}
```

**File Format:** XML or JSON (user choice via export dialog)

**Thread Safety:** File I/O on UI thread or background thread, NOT audio thread.

---

## 4. Parameter Mapping

All parameters use `AudioProcessorValueTreeState` (APVTS) for host automation and preset management.

### 4.1 Motion Engine Parameters

| Parameter ID | Type | Range | Default | Skew | Automation |
|--------------|------|-------|---------|------|------------|
| PATH | Choice | 0-4 (Orbit/Pendulum/Linear/Drift/Custom) | 0 (Orbit) | Linear | Yes |
| SPEED | Float | 0.01 - 20 Hz | 1.0 Hz | Exponential (0.5) | Yes |
| WIDTH | Float | 0 - 360° | 180° | Linear | Yes |
| DEPTH | Float | 0 - 100% | 0% | Linear | Yes |
| TILT | Float | -90 - +90° | 0° | Linear | Yes |
| PHASE | Float | 0 - 360° | 0° | Linear | Yes |
| ELEVATION_ENABLE | Bool | Off/On | Off | N/A | Yes |
| ELEVATION_RANGE | Float | 0 - 90° | 45° | Linear | Yes |
| TEMPO_SYNC | Choice | Off, 1/16, 1/8, 1/4, 1/2, 1, 2, 4 bars | Off | Linear | Yes |

**Parameter Smoothing:**
- Speed, Width, Depth, Tilt, Phase: `juce::SmoothedValue<float>` with 20ms ramp time
- Prevents clicks/zippers when automating motion parameters

### 4.2 Spatial Rendering Parameters

| Parameter ID | Type | Range | Default | Skew | Automation |
|--------------|------|-------|---------|------|------------|
| SPEAKER_LAYOUT | Choice | 0-8 (Stereo/Quad/5.1/7.1/5.1.4/7.1.4/Hex/Oct/Custom) | 0 (Stereo) | Linear | No (state) |
| DISTANCE | Float | 0.1 - 30 m | 1.0 m | Exponential (0.5) | Yes |
| AIR_ABSORPTION | Float | 0 - 100% | 50% | Linear | Yes |
| ATTENUATION_CURVE | Choice | 0-2 (Linear/Inverse/InvSquare) | 1 (Inverse) | Linear | Yes |
| CENTER_DIVERGE | Float | 0 - 100% | 0% | Linear | Yes |

**Note:** `SPEAKER_LAYOUT` is a state variable, not automatable (changing speaker count mid-playback is undefined).

### 4.3 Mix / Source Parameters

| Parameter ID | Type | Range | Default | Skew | Automation |
|--------------|------|-------|---------|------|------------|
| SOURCE_MODE | Choice | 0-1 (Mono/L+R Split) | 0 (Mono) | Linear | Yes |
| LR_OFFSET | Float | 0 - 360° | 180° | Linear | Yes |
| MIX | Float | 0 - 100% | 100% | Linear | Yes |

**Total Automatable Parameters:** 14 (excluding SPEAKER_LAYOUT which is state)

---

## 5. Algorithm Details

### 5.1 Motion Path Algorithms

#### Orbit (Elliptical)
```cpp
float t = phaseAccumulator + (phase * M_PI / 180.0f);
azimuth = centerAzimuth + (width / 2.0f) * std::cos(t);
elevation = elevationEnable ? (centerElevation + elevRange * std::sin(t)) : 0.0f;
distance = baseDistance + depth * 0.5f * (std::sin(t) + 1.0f); // 0→depth modulation
phaseAccumulator += 2.0f * M_PI * speed / sampleRate;
if (phaseAccumulator >= 2.0f * M_PI) phaseAccumulator -= 2.0f * M_PI;
```

#### Pendulum (Linear Swing)
```cpp
float t = phaseAccumulator;
azimuth = centerAzimuth + (width / 2.0f) * std::sin(t); // Single-axis swing
elevation = 0.0f; // Pendulum is horizontal only
distance = baseDistance;
phaseAccumulator += 2.0f * M_PI * speed / sampleRate;
```

#### Linear (Constant Velocity Sweep)
```cpp
azimuth += speed * 360.0f / sampleRate; // degrees per sample
if (azimuth > 180.0f) azimuth -= 360.0f;
elevation = 0.0f;
distance = baseDistance;
```

#### Drift (Perlin Noise)
```cpp
// Low-frequency random walk with low-pass smoothing
noiseState += (randomUniform() - 0.5f) * noiseSpeed;
noiseState *= 0.99f; // Decay towards zero
azimuth += noiseState;
azimuth = wrapAngle(azimuth, -180.0f, 180.0f);
```

#### Custom (User-Drawn Path)
- Path stored as spline control points from WebView canvas
- Interpolate along spline at current phase position
- Requires `juce::Path` or custom spline evaluator

---

### 5.2 VBAP Gain Calculation (Detailed)

**Preprocessing (done once per layout change):**

```cpp
struct VBAPTriangle {
    int speakerIndices[3];
    float inverseMatrix[9]; // 3x3 matrix stored flat
};

std::vector<VBAPTriangle> triangles;

void preprocessSpeakerLayout(const SpeakerLayout& layout) {
    // 1. Convert speaker positions to Cartesian unit vectors
    std::vector<float> cartesian(layout.speakers.size() * 3);
    for (size_t i = 0; i < layout.speakers.size(); ++i) {
        float azRad = layout.speakers[i].azimuth * M_PI / 180.0f;
        float elRad = layout.speakers[i].elevation * M_PI / 180.0f;
        cartesian[i*3 + 0] = std::cos(elRad) * std::cos(azRad); // X
        cartesian[i*3 + 1] = std::cos(elRad) * std::sin(azRad); // Y
        cartesian[i*3 + 2] = std::sin(elRad);                    // Z
    }

    // 2. Compute Delaunay triangulation (use SAF or qhull)
    int* triangleIndices = nullptr;
    int numTriangles = 0;
    findLsTriplets(/* speaker dirs, num speakers, output */, &triangleIndices, &numTriangles);

    // 3. For each triangle, compute and invert the 3x3 matrix
    triangles.resize(numTriangles);
    for (int t = 0; t < numTriangles; ++t) {
        int i1 = triangleIndices[t*3 + 0];
        int i2 = triangleIndices[t*3 + 1];
        int i3 = triangleIndices[t*3 + 2];

        float L[9] = {
            cartesian[i1*3+0], cartesian[i1*3+1], cartesian[i1*3+2],
            cartesian[i2*3+0], cartesian[i2*3+1], cartesian[i2*3+2],
            cartesian[i3*3+0], cartesian[i3*3+1], cartesian[i3*3+2]
        };

        invert3x3(L, triangles[t].inverseMatrix);
        triangles[t].speakerIndices[0] = i1;
        triangles[t].speakerIndices[1] = i2;
        triangles[t].speakerIndices[2] = i3;
    }
}
```

**Real-Time Gain Computation:**

```cpp
void computeVBAPGains(float azimuth, float elevation, float* gains, int numSpeakers) {
    // 1. Convert source direction to unit vector
    float azRad = azimuth * M_PI / 180.0f;
    float elRad = elevation * M_PI / 180.0f;
    float px = std::cos(elRad) * std::cos(azRad);
    float py = std::cos(elRad) * std::sin(azRad);
    float pz = std::sin(elRad);

    // 2. Find active triangle (test all triangles for non-negative gains)
    const VBAPTriangle* activeTriangle = nullptr;
    float g[3];
    for (const auto& tri : triangles) {
        // g = p^T * L^-1 (matrix-vector multiply)
        g[0] = px * tri.inverseMatrix[0] + py * tri.inverseMatrix[1] + pz * tri.inverseMatrix[2];
        g[1] = px * tri.inverseMatrix[3] + py * tri.inverseMatrix[4] + pz * tri.inverseMatrix[5];
        g[2] = px * tri.inverseMatrix[6] + py * tri.inverseMatrix[7] + pz * tri.inverseMatrix[8];

        if (g[0] >= 0.0f && g[1] >= 0.0f && g[2] >= 0.0f) {
            activeTriangle = &tri;
            break;
        }
    }

    if (activeTriangle == nullptr) {
        // Fallback: source is outside convex hull (shouldn't happen for unit sphere)
        std::fill(gains, gains + numSpeakers, 0.0f);
        return;
    }

    // 3. Normalize gains: g = g / ||g||
    float norm = std::sqrt(g[0]*g[0] + g[1]*g[1] + g[2]*g[2]);
    g[0] /= norm;
    g[1] /= norm;
    g[2] /= norm;

    // 4. Assign gains to speakers
    std::fill(gains, gains + numSpeakers, 0.0f);
    gains[activeTriangle->speakerIndices[0]] = g[0];
    gains[activeTriangle->speakerIndices[1]] = g[1];
    gains[activeTriangle->speakerIndices[2]] = g[2];
}
```

**Optimization:** Pre-generate gain tables at 1° resolution for static lookups (avoids triangle search per block).

---

### 5.3 Distance Model Implementation

```cpp
class DistanceModel {
public:
    void prepare(double sampleRate) {
        this->sampleRate = sampleRate;
        lpf.prepare({sampleRate, (uint32)512, 1});
    }

    void updateDistance(float distance, float airAbsorption) {
        this->distance = distance;

        // Compute LPF cutoff
        float cutoff = 20000.0f / (1.0f + airAbsorption * (distance / 10.0f));
        cutoff = std::clamp(cutoff, 100.0f, 20000.0f);

        *lpf.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoff);
    }

    float processGain(float attenuationCurve) {
        // Attenuation curves
        switch (attenuationCurve) {
            case AttenuationCurve::Linear:
                return std::clamp(1.0f - (distance - 0.1f) / 30.0f, 0.0f, 1.0f);
            case AttenuationCurve::Inverse:
                return 1.0f / std::max(distance, 0.1f);
            case AttenuationCurve::InverseSquare:
                return 1.0f / std::max(distance * distance, 0.01f);
        }
    }

    float processSample(float sample) {
        return lpf.processSample(sample);
    }

private:
    float distance = 1.0f;
    double sampleRate = 48000.0;
    juce::dsp::IIR::Filter<float> lpf;
};
```

---

## 6. Integration Points

### 6.1 Component Dependencies

| Component | Depends On | Reason |
|-----------|-----------|--------|
| VBAP Renderer | Speaker Layout System | Needs speaker positions for triangulation |
| Motion Engine | Host Tempo (optional) | Tempo sync requires BPM from host |
| Distance Model | Motion Engine | Distance value computed by motion engine |
| Auto-Downmix | VBAP Renderer + Bus Configuration | Needs VBAP gains and actual output channel count |
| UI Visualizer | Motion Engine | Reads position for animated orbital display |

### 6.2 Parameter Interactions

| Parameter A | Affects | Parameter B | Reason |
|-------------|---------|-------------|--------|
| TEMPO_SYNC | Overrides | SPEED | When tempo sync enabled, speed is BPM-derived |
| ELEVATION_ENABLE | Enables | ELEVATION_RANGE | Range inactive if elevation disabled |
| SOURCE_MODE = L+R Split | Enables | LR_OFFSET | Offset meaningless in mono mode |
| SPEAKER_LAYOUT | Constrains | Output channel count | Layout defines required channels |

### 6.3 Processing Order

1. **Parameter Updates** (start of `processBlock`): Read APVTS, smooth parameter changes
2. **Motion State Update** (per-block): Increment LFO phase, compute azimuth/elevation/distance
3. **Source Preparation** (per-block): Apply source mode (mono sum or L+R split)
4. **VBAP Gain Calculation** (per-block): Compute speaker gains for current source position(s)
5. **Per-Sample Processing** (inner loop):
   - Apply distance gain
   - Apply air absorption LPF
   - Accumulate to speaker feeds using VBAP gains
6. **Auto-Downmix** (per-block, if active): Fold speaker feeds to available channels
7. **Mix** (per-block): Blend wet (spatialized) with dry (input passthrough)

**Critical:** VBAP gain calculation happens ONCE per block (not per sample) for efficiency.

### 6.4 Thread Boundaries

| Thread | Responsibilities | Access Pattern |
|--------|------------------|----------------|
| Audio Thread | All DSP processing | Read parameters (APVTS atomic), write audio buffers |
| UI Thread | Parameter updates via APVTS, speaker layout editing | Write parameters (APVTS thread-safe), read position snapshots |
| Background Thread (optional) | VBAP triangulation on layout change | Write preprocessed data (mutex-protected), no audio buffer access |

**Synchronization:**
- APVTS handles parameter thread safety automatically
- Motion state read by UI uses atomic snapshot: `std::atomic<MotionSnapshot> motionSnapshot;`
- VBAP triangulation uses `std::mutex` around triangle data swap

---

## 7. Implementation Risks

### 7.1 VBAP Triangulation Complexity

**Risk Level:** HIGH

**Issue:** Delaunay triangulation on sphere is non-trivial. Naive implementations fail for irregular layouts.

**Fallback Architecture:**
- **Plan A:** Use SAF `saf_vbap` module (production-ready, tested)
- **Plan B:** Use qhull library for convex hull on unit sphere
- **Plan C:** 2D-only VBAP (pair-wise panning, no elevation support) - simpler, 90% use cases

**Mitigation:** Start with SAF integration. If build complexity is prohibitive, fall back to 2D-only VBAP for MVP.

### 7.2 Multi-Channel Bus Negotiation

**Risk Level:** MEDIUM

**Issue:** DAWs have inconsistent multi-channel support. Logic Pro and Reaper support arbitrary channel counts, but Ableton is stereo-only.

**Fallback Architecture:**
- Always provide stereo output option via auto-downmix
- Document DAW compatibility matrix in user manual
- Auto-detect channel count in `prepareToPlay()` and display warning if mismatch

**Mitigation:** Test in Logic Pro (best multichannel support) and Reaper (VST3 multichannel) early. Ableton users get stereo-only but plugin still works.

### 7.3 Real-Time Performance (High Speaker Counts)

**Risk Level:** MEDIUM

**Issue:** 24-speaker layout with 2 sources = 48 gain calculations per block + 48 channels of audio accumulation.

**Fallback Architecture:**
- Use pre-computed VBAP gain tables at 1° resolution (avoids triangle search)
- SIMD optimization for gain accumulation (`juce::FloatVectorOperations::multiply()`)
- Limit max speaker count to 16 for MVP

**Mitigation:** Profile early. VBAP gain calculation is ~1-5μs per source on modern CPU. Even 24 speakers × 2 sources = ~10μs, negligible in 512-sample block (~10ms at 48kHz).

### 7.4 WebView UI Complexity (Orbital Visualizer)

**Risk Level:** MEDIUM

**Issue:** Real-time animated orbital path requires JavaScript canvas animation at 60fps synced with audio thread position.

**Fallback Architecture:**
- Static speaker layout editor only (no animated orbit visualization)
- OR: Simple 2D top-down view with source position dot (no path trails)

**Mitigation:** Start with static layout editor. Add animation in Stage 3 GUI polish if time permits.

### 7.5 Custom Path Editor

**Risk Level:** HIGH

**Issue:** User-drawable paths require spline editing in WebView, path data persistence, and spline evaluation on audio thread.

**Fallback Architecture:**
- Omit custom path for MVP
- Provide Orbit, Pendulum, Linear, Drift only (predefined algorithms)

**Mitigation:** Custom path is a "nice-to-have." Ship v1.0 without it, add in v1.1 if there's demand.

---

## 8. Architecture Decisions

### 8.1 VBAP vs. Ambisonics

**Decision:** Use VBAP (direct amplitude panning) instead of Ambisonics encoding + decoding.

**Why:**
- VBAP is simpler (no spherical harmonics, no encoding/decoding stages)
- Lower CPU cost (multiplication vs. convolution)
- Works natively with any speaker layout (Ambisonics assumes regular arrays)
- Custom non-equidistant layouts are first-class citizens in VBAP
- No order limitations (Ambisonics quality degrades at low orders for sparse layouts)

**Tradeoff:** Cannot rotate the scene post-processing (Ambisonics allows this). For O-Orbit, rotation is handled by motion engine, not decoder.

### 8.2 SAF Integration vs. Scratch Implementation

**Decision:** Use SAF (Spatial Audio Framework) for VBAP, implement motion engine + distance model from scratch.

**Why:**
- VBAP triangulation is complex and error-prone; SAF is battle-tested
- SAF license (ISC) is permissive and commercial-friendly
- SAF dependency (Apple Accelerate on macOS) is zero-friction
- Motion engine and distance model are simple enough for custom code

**Tradeoff:** Build complexity on Windows (requires Intel MKL or OpenBLAS). Mitigated by CI/CD setup scripts.

### 8.3 Flexible Channel Count vs. Named Layouts

**Decision:** Accept any channel count 2-24, match to user-configured speaker layout.

**Why:**
- Named JUCE layouts (e.g., `create7point1point4()`) are rigid
- Custom speaker arrays don't map to named layouts
- Flexibility is O-Orbit's differentiator

**Tradeoff:** More complex parameter handling (must validate channel count matches layout). User education required.

### 8.4 Per-Block vs. Per-Sample VBAP

**Decision:** VBAP gain calculation per-block, apply gains per-sample.

**Why:**
- VBAP triangle search is expensive (~10-50μs depending on layout complexity)
- Gains change slowly (motion at 0.01-20 Hz)
- Linear interpolation between blocks prevents zipper noise

**Tradeoff:** Motion at very high speeds (>10 Hz) may cause slight spatial aliasing. Acceptable for target use cases.

---

## 9. Special Considerations

### 9.1 Thread Safety

**Parameter Access:**
- APVTS parameters are atomic and thread-safe by design
- Audio thread reads parameter values at start of `processBlock()`
- No direct writes from UI thread to audio thread state

**Motion State Snapshot:**
```cpp
struct MotionSnapshot {
    float azimuth;
    float elevation;
    float distance;
};

std::atomic<MotionSnapshot> motionSnapshotForUI;

// Audio thread (write):
void processBlock(...) {
    motionState.update();
    motionSnapshotForUI.store(motionState.snapshot(), std::memory_order_relaxed);
}

// UI thread (read):
void timerCallback() {
    auto snapshot = motionSnapshotForUI.load(std::memory_order_relaxed);
    updateVisualizerPosition(snapshot.azimuth, snapshot.elevation);
}
```

**Speaker Layout Modification:**
- Editing speaker positions happens on UI thread
- Background thread recomputes VBAP triangulation
- Atomic swap of preprocessed triangle data when ready
- Mutex protects triangle vector during swap

### 9.2 Performance Targets

**Target CPU Usage (per instance):**
- Stereo output, 1 source: <1% (Intel i5 or equivalent)
- 7.1.4 output (12 channels), 2 sources: <3%
- 24-channel custom output, 2 sources: <5%

**Latency:**
- Zero added latency (VBAP is non-blocking, no lookahead required)
- Distance LPF has inherent 1-sample delay (negligible)

**Memory:**
- Precomputed VBAP triangles: ~1-10 KB depending on layout
- Distance LPF state: ~16 bytes per source
- Total plugin footprint: <100 KB (excluding JUCE/SAF dependencies)

### 9.3 Denormal Prevention

Distance model LPF and gain calculations can produce denormals when source is very far or gains are near-zero.

**Mitigation:**
```cpp
// In processBlock, enable FTZ/DAZ:
juce::ScopedNoDenormals noDenormals;

// In distance gain calculation:
float distGain = 1.0f / std::max(distance, 0.1f); // Clamp minimum distance

// In VBAP gain application:
for (int s = 0; s < numSpeakers; ++s) {
    float g = gains[s];
    if (g < 1e-6f) g = 0.0f; // Zero out tiny gains
    speakerFeed[s] += sample * g;
}
```

### 9.4 Sample Rate Independence

All motion engine frequencies and LPF cutoffs must scale with sample rate.

**Implementation:**
```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    this->sampleRate = sampleRate;
    motionEngine.setSampleRate(sampleRate);
    distanceModel.prepare(sampleRate);
}

// In motion engine:
phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
```

### 9.5 Latency Reporting

VBAP introduces zero latency. Distance LPF is 1 sample (negligible).

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    setLatencySamples(0); // No latency compensation needed
}
```

---

## 10. Research References

### VBAP Algorithm

- **Ville Pulkki's VBAP Paper:** [VECTOR-BASE AMPLITUDE PANNING LIBRARY](http://research.spa.aalto.fi/projects/vbap-lib/vbap.html)
- **VBAP Implementation (polarch):** [GitHub - Vector-Base-Amplitude-Panning](https://github.com/polarch/Vector-Base-Amplitude-Panning)
- **VBAP Overview:** [Andrew McWilliams - VBAP](https://jahya.net/blog/vector-base-amplitude-panning/)
- **SAF VBAP Module:** [SAF: saf_vbap](https://leomccormack.github.io/Spatial_Audio_Framework/group___v_b_a_p.html)

### Spatial Audio Framework (SAF)

- **SAF Repository:** [GitHub - Spatial_Audio_Framework](https://github.com/leomccormack/Spatial_Audio_Framework)
- **SAF Documentation:** [SAF API Reference](https://leomccormack.github.io/Spatial_Audio_Framework/)
- **SPARTA Plugins (SAF-based):** [SPARTA - Spatial Audio Real-Time Applications](https://leomccormack.github.io/sparta-site/)

### JUCE Multi-Channel Audio

- **JUCE Bus Layouts Tutorial:** [Configuring the right bus layouts for your plugins](https://docs.juce.com/master/tutorial_audio_bus_layouts.html)
- **JUCE AudioProcessor Bus API:** [JUCE: juce::AudioProcessor::Bus Class Reference](https://docs.juce.com/master/classAudioProcessor_1_1Bus.html)
- **Local Research:** `research/juce8-multichannel-spatial-audio.md`
- **Local Research:** `research/sound-spatialization-algorithms.md`
- **Local Research:** `research/saf-juce-integration-guide.md`

### Distance Modeling

- **Air Absorption DSP:** [Air absorption of sound as a digital filter – Part 1: Theory](https://codeandsound.wordpress.com/2014/08/21/absorption-of-sound-by-air-and-its-implementation-as-a-filter-part-1-theory/)
- **Atmospheric Absorption Approximation:** [Approximating Atmospheric Absorption With a Simple Filter](https://computingandrecording.wordpress.com/2017/07/05/approximating-atmospheric-absorption-with-a-simple-filter/)
- **ISO 9613-1:** Attenuation of sound during propagation outdoors (referenced in research)

### Professional Spatial Plugins (Market Research)

- **Brauer Motion (Waves):** [Brauer Motion – Circular Auto-Panner](https://www.waves.com/plugins/brauer-motion)
- **SPARTA Plugin Suite:** [Sparta vst | TripinLab](https://www.tripinlab.com/documentation/sparta-vst/)
- **Best Spatial Audio Plugins 2025:** [7 Best Spatial Audio Plugins of 2025 - Reviewed & Ranked](https://www.audiocube.app/blog/spatial-audio-plugin)

### LFO and Modulation

- **LFO Plugin Research:** [6 Best LFO Plugins To Add Life To Mix 2026](https://pluginoise.com/8-best-lfo-plugins/)
- **Tempo Sync Implementation:** [Tempo sync'd LFO? - Audio Plugins - JUCE](https://forum.juce.com/t/tempo-syncd-lfo/4496)

---

## Document Revision History

| Date | Version | Changes |
|------|---------|---------|
| 2026-02-09 | 1.0 | Initial architecture specification |

---

**End of Architecture Specification**
