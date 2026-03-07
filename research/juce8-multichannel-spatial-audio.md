---
title: "JUCE 8 Multichannel & Spatial Audio for Plugins"
created: 2026-02-08
domain: spatial-audio
type: guide
keywords:
  - juce
  - multichannel
  - spatial-audio
  - ambisonics
  - plugin-architecture
  - bus-layout
---
# JUCE 8 Multichannel & Spatial Audio for Plugins

## Research Date: 2026-02-07
## JUCE Version: 8.0.4

---

## 1. AudioChannelSet Configurations

JUCE 8 provides a comprehensive set of named channel layouts via `juce::AudioChannelSet`. Every layout is a collection of typed channel slots (not just a channel count).

### Complete Layout Reference

| Layout | Channels | JUCE Static Method | VST3 | AAX | CoreAudio/AU |
|--------|----------|---------------------|-------|-----|--------------|
| Mono | 1 | `mono()` | kMonoAAX | AAX_eStemFormat_Mono | kAudioChannelLayoutTag_Mono |
| Stereo | 2 | `stereo()` | kStereo | AAX_eStemFormat_Stereo | kAudioChannelLayoutTag_Stereo |
| LCR | 3 | `createLCR()` | k30Cine | AAX_eStemFormat_LCR | kAudioChannelLayoutTag_MPEG_3_0_A |
| Quad | 4 | `quadraphonic()` | k40Music | AAX_eStemFormat_Quad | kAudioChannelLayoutTag_Quadraphonic |
| 5.0 | 5 | `create5point0()` | k50 | AAX_eStemFormat_5_0 | kAudioChannelLayoutTag_MPEG_5_0_A |
| 5.1 | 6 | `create5point1()` | k51 | AAX_eStemFormat_5_1 | kAudioChannelLayoutTag_MPEG_5_1_A |
| 7.0 | 7 | `create7point0()` | k70Music | AAX_eStemFormat_7_0_DTS | kAudioChannelLayoutTag_AudioUnit_7_0 |
| 7.1 | 8 | `create7point1()` | k71CineSideFill | AAX_eStemFormat_7_1_DTS | kAudioChannelLayoutTag_MPEG_7_1_C |
| 5.0.2 | 7 | `create5point0point2()` | n/a | AAX_eStemFormat_5_0_2 | n/a |
| 5.1.2 | 8 | `create5point1point2()` | n/a | n/a | kAudioChannelLayoutTag_Atmos_5_1_2 |
| 5.1.4 | 10 | `create5point1point4()` | n/a | n/a | kAudioChannelLayoutTag_Atmos_5_1_4 |
| 7.0.2 | 9 | `create7point0point2()` | n/a | AAX_eStemFormat_7_0_2 | n/a |
| 7.1.2 | 10 | `create7point1point2()` | k71_2 | AAX_eStemFormat_7_1_2 | kAudioChannelLayoutTag_Atmos_7_1_2 |
| **7.1.4** | **12** | **`create7point1point4()`** | **k71_4** | **n/a** | **kAudioChannelLayoutTag_Atmos_7_1_4** |
| 7.1.6 | 14 | `create7point1point6()` | k71_6 | n/a | n/a |
| 9.1.4 | 14 | `create9point1point4()` | k91_4_W | AAX_eStemFormat_9_1_4 | n/a |
| 9.1.6 | 16 | `create9point1point6()` | k91_6_W | n/a | kAudioChannelLayoutTag_Atmos_9_1_6 |
| Ambi 1st | 4 | `ambisonic(1)` | kAmbi1stOrderACN | AAX_eStemFormat_Ambi_1_ACN | kAudioChannelLayoutTag_HOA_ACN_SN3D |
| Ambi 2nd | 9 | `ambisonic(2)` | kAmbi2ndOrderACN | AAX_eStemFormat_Ambi_2_ACN | kAudioChannelLayoutTag_HOA_ACN_SN3D |
| Ambi 3rd | 16 | `ambisonic(3)` | kAmbi3rdOrderACN | AAX_eStemFormat_Ambi_3_ACN | kAudioChannelLayoutTag_HOA_ACN_SN3D |
| Ambi 4th | 25 | `ambisonic(4)` | kAmbi4thOrderACN | AAX_eStemFormat_Ambi_4_ACN | kAudioChannelLayoutTag_HOA_ACN_SN3D |
| Ambi 5th | 36 | `ambisonic(5)` | kAmbi5thOrderACN | AAX_eStemFormat_Ambi_5_ACN | kAudioChannelLayoutTag_HOA_ACN_SN3D |
| Ambi 6th | 49 | `ambisonic(6)` | kAmbi6thOrderACN | AAX_eStemFormat_Ambi_6_ACN | kAudioChannelLayoutTag_HOA_ACN_SN3D |
| Ambi 7th | 64 | `ambisonic(7)` | kAmbi7thOrderACN | AAX_eStemFormat_Ambi_7_ACN | kAudioChannelLayoutTag_HOA_ACN_SN3D |

### 7.1.4 Atmos Channel Composition (12 channels)

Declared in `/Users/taylorbrook/JUCE/modules/juce_audio_basics/buffers/juce_AudioChannelSet.cpp` line 580:
```
left, right, centre, LFE, leftSurroundSide, rightSurroundSide,
leftSurroundRear, rightSurroundRear, topFrontLeft, topFrontRight,
topRearLeft, topRearRight
```

---

## 2. isBusesLayoutSupported() - Complete Code Examples

### Example A: Spatial Granular Plugin (Stereo In -> Multiple Output Formats)

```cpp
class SpatialGranularProcessor : public juce::AudioProcessor
{
public:
    SpatialGranularProcessor()
        : AudioProcessor (BusesProperties()
            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::ambisonic (1), true))
    {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        auto mainInput  = layouts.getMainInputChannelSet();
        auto mainOutput = layouts.getMainOutputChannelSet();

        // Always require input
        if (mainInput.isDisabled() || mainOutput.isDisabled())
            return false;

        // Accept mono or stereo input
        if (mainInput != juce::AudioChannelSet::mono()
            && mainInput != juce::AudioChannelSet::stereo())
            return false;

        // Output: stereo, quad, 5.1, 7.1, 7.1.4 (Atmos), or ambisonics 1st-3rd order
        if (mainOutput == juce::AudioChannelSet::stereo())       return true;
        if (mainOutput == juce::AudioChannelSet::quadraphonic()) return true;
        if (mainOutput == juce::AudioChannelSet::create5point1()) return true;
        if (mainOutput == juce::AudioChannelSet::create7point1()) return true;
        if (mainOutput == juce::AudioChannelSet::create7point1point4()) return true;
        if (mainOutput == juce::AudioChannelSet::ambisonic (1))  return true;
        if (mainOutput == juce::AudioChannelSet::ambisonic (2))  return true;
        if (mainOutput == juce::AudioChannelSet::ambisonic (3))  return true;

        return false;
    }
};
```

### Example B: Ambisonics Encoder (Mono -> Ambisonics 1st-3rd Order)

```cpp
class AmbisonicEncoderProcessor : public juce::AudioProcessor
{
public:
    AmbisonicEncoderProcessor()
        : AudioProcessor (BusesProperties()
            .withInput  ("Input",  juce::AudioChannelSet::mono(), true)
            .withOutput ("Output", juce::AudioChannelSet::ambisonic (1), true))
    {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        auto mainInput  = layouts.getMainInputChannelSet();
        auto mainOutput = layouts.getMainOutputChannelSet();

        if (mainInput.isDisabled() || mainOutput.isDisabled())
            return false;

        // Only mono input for encoding a single source
        if (mainInput != juce::AudioChannelSet::mono())
            return false;

        // Output must be a valid ambisonic order (1-3)
        int order = mainOutput.getAmbisonicOrder();
        return order >= 1 && order <= 3;
    }
};
```

### Example C: Multichannel Effect (Match In/Out, Multiple Formats)

```cpp
class MultichannelEffectProcessor : public juce::AudioProcessor
{
public:
    MultichannelEffectProcessor()
        : AudioProcessor (BusesProperties()
            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        auto mainInput  = layouts.getMainInputChannelSet();
        auto mainOutput = layouts.getMainOutputChannelSet();

        // Input must match output for an effect plugin
        if (mainInput != mainOutput)
            return false;

        // Support these specific formats
        if (mainOutput == juce::AudioChannelSet::mono())              return true;
        if (mainOutput == juce::AudioChannelSet::stereo())            return true;
        if (mainOutput == juce::AudioChannelSet::quadraphonic())      return true;
        if (mainOutput == juce::AudioChannelSet::create5point1())     return true;
        if (mainOutput == juce::AudioChannelSet::create7point1())     return true;
        if (mainOutput == juce::AudioChannelSet::create7point1point2()) return true;
        if (mainOutput == juce::AudioChannelSet::create7point1point4()) return true;

        // Also support ambisonics pass-through
        int order = mainOutput.getAmbisonicOrder();
        if (order >= 1 && order <= 3)
            return true;

        return false;
    }
};
```

### Example D: Format Converter (7.1.4 Atmos <-> Ambisonics)

```cpp
class AtmosToAmbiConverter : public juce::AudioProcessor
{
public:
    AtmosToAmbiConverter()
        : AudioProcessor (BusesProperties()
            .withInput  ("Input",  juce::AudioChannelSet::create7point1point4(), true)
            .withOutput ("Output", juce::AudioChannelSet::ambisonic (2), true))
    {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        auto mainInput  = layouts.getMainInputChannelSet();
        auto mainOutput = layouts.getMainOutputChannelSet();

        if (mainInput.isDisabled() || mainOutput.isDisabled())
            return false;

        // Input: 7.1.4 or ambisonics
        bool validInput = (mainInput == juce::AudioChannelSet::create7point1point4()
                        || mainInput.getAmbisonicOrder() >= 1);

        // Output: 7.1.4 or ambisonics
        bool validOutput = (mainOutput == juce::AudioChannelSet::create7point1point4()
                         || mainOutput.getAmbisonicOrder() >= 1);

        return validInput && validOutput;
    }
};
```

---

## 3. Ambisonics Support in JUCE 8

### Channel Counts Per Order

The formula is: `numChannels = (order + 1)^2`

| Order | Channels | JUCE Call | Use Case |
|-------|----------|-----------|----------|
| 0 | 1 | `ambisonic(0)` | Omnidirectional (mono) |
| 1 (FOA) | 4 | `ambisonic(1)` | Basic 3D - W, Y, Z, X (ACN 0-3) |
| 2 | 9 | `ambisonic(2)` | Medium resolution |
| 3 (HOA) | 16 | `ambisonic(3)` | Good resolution for VR/AR |
| 4 | 25 | `ambisonic(4)` | High resolution |
| 5 | 36 | `ambisonic(5)` | Very high resolution |
| 6 | 49 | `ambisonic(6)` | Ultra high resolution |
| 7 | 64 | `ambisonic(7)` | Maximum supported by JUCE |

### JUCE Ambisonics Format: ACN + SN3D

JUCE uses the standard **AmbiX** format:
- **Channel ordering**: ACN (Ambisonic Channel Numbering)
- **Normalization**: SN3D (Schmidt semi-normalization)
- This is the de facto standard for ambisonics interchange

From `juce_AudioChannelSet.h` line 372-376:
```cpp
/** Creates a set for ACN, SN3D normalised ambisonic surround setups with a given order.
    Is equivalent to: kAmbiXXXOrderACN (VST), AAX_eStemFormat_Ambi_XXX_ACN (AAX),
    kAudioChannelLayoutTag_HOA_ACN_SN3D (CoreAudio)
*/
static AudioChannelSet JUCE_CALLTYPE ambisonic (int order = 1);
```

### Querying Ambisonics

```cpp
// Check if a layout is ambisonics
int order = channelSet.getAmbisonicOrder();
if (order >= 0) {
    // It's an ambisonic layout of this order
}

// Find which order fits a channel count
int order = AudioChannelSet::getAmbisonicOrderForNumChannels(16); // returns 3

// W, X, Y, Z aliases for 1st order (B-format legacy)
// ambisonicW == ambisonicACN0 (W - omnidirectional)
// ambisonicY == ambisonicACN1 (Y - left/right)
// ambisonicZ == ambisonicACN2 (Z - up/down)
// ambisonicX == ambisonicACN3 (X - front/back)
```

### What JUCE Does NOT Provide

JUCE provides **channel layout management** for ambisonics but does NOT include:
- Ambisonics encoding (spherical harmonic calculations)
- Ambisonics decoding to speaker feeds
- VBAP (Vector Base Amplitude Panning)
- DBAP (Distance-Based Amplitude Panning)
- Binaural rendering
- Head-tracking integration
- Any spatial audio DSP algorithms

The `juce::dsp::Panner` class is **stereo only** (2 channel output, 1-2 channel input). It is NOT suitable for spatial audio.

**You must implement or integrate spatial DSP yourself.** See Section 7 for recommended libraries.

---

## 4. DAW Compatibility Matrix

### Channel Layout Support by DAW

| Layout | Logic Pro (AU) | Reaper (VST3) | Nuendo (VST3) | Pro Tools (AAX) | Ableton (VST3/AU) |
|--------|---------------|---------------|---------------|-----------------|-------------------|
| Stereo | Yes | Yes | Yes | Yes | Yes |
| Quad | Yes | Yes | Yes | Yes | No (stereo max) |
| 5.1 | Yes | Yes | Yes | Yes | No |
| 7.1 | Yes | Yes | Yes | Yes | No |
| 7.1.2 | Yes (10.7+) | Yes | Yes | Yes | No |
| **7.1.4** | **Yes (10.7+)** | **Yes** | **Yes** | **No** | **No** |
| 7.1.6 | No | Yes | Yes | No | No |
| 9.1.6 | Yes (recent) | Yes | Yes | No | No |
| Ambi 1st | Yes | Yes | Yes | Yes | No |
| Ambi 2nd | Yes | Yes | Yes | Yes | No |
| Ambi 3rd | Yes | Yes | Yes | Yes | No |
| Ambi 4th+ | Partial | **Issues** | Partial | Yes | No |

### Plugin Format Support for Multichannel

| Format | Max Channel Count | Ambi Support | Atmos Beds | Notes |
|--------|-------------------|--------------|------------|-------|
| **VST3** | 64+ (spec) | Up to 7th order (64ch) | 7.1.4, 9.1.6 | Best multichannel support; 5th+ order had issues in VST3 SDK 3.7.8 |
| **AU (v2/v3)** | 64+ | Via HOA_ACN_SN3D tag | 7.1.4, 9.1.6 | Logic Pro AU validation can be strict |
| **AAX** | 64 | 1st-7th order | 7.1.2 max | Pro Tools limited to 7.1.2 for Atmos beds |
| **LV2** | Unlimited | Yes | Via port groups | Best for open-source/Linux |

### Known Issues and Gotchas

1. **VST3 High-Order Ambisonics (4th-7th order)**: The VST3 SDK 3.7.8 had a 24-channel limit for ambisonics speaker arrangements. JUCE 8 bundled SDK now supports 5th-7th order via `kAmbi5thOrderACN` etc., but older Reaper versions may still cap at 24 channels.

2. **Logic Pro AU Channel Ordering**: Logic may use different channel arrangement variants ("A" vs "B" layouts) depending on ARM vs x86 architecture. Test on both if supporting Apple Silicon.

3. **Logic Pro AU Cache**: Must clear AU cache when changing channel layouts (already in project build guidelines).

4. **Reaper Channel Remapping**: When a plugin changes output format (e.g., 7.1.4 to ambisonics), Reaper may incorrectly remap channels based on the original default layout rather than the new format.

5. **Pro Tools 7.1.4**: AAX does NOT support 7.1.4 beds. Maximum Atmos bed in Pro Tools is 7.1.2.

6. **Ableton Live**: Only supports stereo plugins. Multichannel plugins will not work.

---

## 5. Dynamic Channel Routing & Bus Negotiation

### How Bus Negotiation Works

The plugin is **passive** in bus negotiation. It cannot set its own layout -- only the host can. The flow is:

1. Plugin declares default layout in constructor via `BusesProperties`
2. Host queries `isBusesLayoutSupported()` to discover valid configurations
3. Host calls `setBusesLayout()` which triggers `canApplyBusesLayout()` then `applyBusLayouts()`
4. Plugin adapts in `prepareToPlay()` based on the negotiated layout

### Querying Current Layout at Runtime

```cpp
void prepareToPlay (double sampleRate, int samplesPerBlock) override
{
    auto outputLayout = getBusesLayout().getMainOutputChannelSet();

    if (outputLayout == juce::AudioChannelSet::ambisonic (1))
    {
        // Configure for 4-channel FOA output
        numOutputChannels = 4;
        outputMode = OutputMode::Ambisonics1;
    }
    else if (outputLayout == juce::AudioChannelSet::create7point1point4())
    {
        // Configure for 12-channel Atmos bed
        numOutputChannels = 12;
        outputMode = OutputMode::Atmos714;
    }
    else if (outputLayout == juce::AudioChannelSet::stereo())
    {
        numOutputChannels = 2;
        outputMode = OutputMode::Stereo;
    }
    // ... allocate buffers based on numOutputChannels
}
```

### Accessing Individual Bus Channels in processBlock

```cpp
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
{
    // Get the main output bus buffer
    auto mainOutput = getBusBuffer (buffer, false, 0);

    int totalChannels = mainOutput.getNumChannels();

    // For ambisonics output, write spherical harmonics directly
    if (outputMode == OutputMode::Ambisonics1)
    {
        float* w = mainOutput.getWritePointer (0); // ACN0 - omnidirectional
        float* y = mainOutput.getWritePointer (1); // ACN1 - Y axis (left/right)
        float* z = mainOutput.getWritePointer (2); // ACN2 - Z axis (up/down)
        float* x = mainOutput.getWritePointer (3); // ACN3 - X axis (front/back)

        for (int s = 0; s < buffer.getNumSamples(); ++s)
        {
            // Your spatial grain synthesis here...
            // Encode grain at (azimuth, elevation) into ambisonics:
            // W = signal
            // Y = signal * sin(azimuth) * cos(elevation)
            // Z = signal * sin(elevation)
            // X = signal * cos(azimuth) * cos(elevation)
        }
    }
}
```

### Multi-Bus Plugins (Sidechain + Spatial Output)

```cpp
SpatialGranularProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
        .withInput  ("Sidechain", juce::AudioChannelSet::mono(), false)  // optional sidechain
        .withOutput ("Output",    juce::AudioChannelSet::ambisonic (1), true))
{}

bool isBusesLayoutSupported (const BusesLayout& layouts) const override
{
    auto mainInput  = layouts.getMainInputChannelSet();
    auto mainOutput = layouts.getMainOutputChannelSet();
    auto sidechain  = layouts.getChannelSet (true, 1);  // input bus index 1

    if (mainInput.isDisabled() || mainOutput.isDisabled())
        return false;

    // Main input: mono or stereo
    if (mainInput.size() < 1 || mainInput.size() > 2)
        return false;

    // Sidechain: disabled or mono (optional)
    if (!sidechain.isDisabled() && sidechain != juce::AudioChannelSet::mono())
        return false;

    // Output: ambisonics or surround
    return mainOutput.getAmbisonicOrder() >= 1
        || mainOutput == juce::AudioChannelSet::create7point1point4()
        || mainOutput == juce::AudioChannelSet::stereo();
}
```

---

## 6. JUCE Source Code Audit: Spatial Audio Content

### Files Examined

| File | Content |
|------|---------|
| `juce_audio_basics/buffers/juce_AudioChannelSet.h` | All channel layouts, ambisonic channel types (ACN0-63), up to 7th order |
| `juce_audio_basics/buffers/juce_AudioChannelSet.cpp` | Layout construction, `ambisonic(order)` implementation using `(order+1)^2` formula |
| `juce_audio_basics/native/juce_CoreAudioLayouts_mac.h` | CoreAudio tag mapping, `kAudioChannelLayoutTag_HOA_ACN_SN3D` for ambisonics |
| `juce_audio_processors/format_types/juce_VST3Common.h` | VST3 speaker arrangement mapping, `Ambisonics::mappings` for 5th-7th order |
| `juce_audio_plugin_client/juce_audio_plugin_client_AAX.cpp` | AAX stem format mapping for ambisonics 1st-7th order |
| `juce_audio_processors/processors/juce_AudioProcessor.h` | `BusesLayout`, `BusesProperties`, `isBusesLayoutSupported()`, bus negotiation API |
| `juce_dsp/processors/juce_Panner.h` | **Stereo-only panner**. NOT suitable for spatial audio. |

### What Exists in JUCE

- Full channel type definitions for all surround and Atmos formats up to 9.1.6
- Ambisonic channel types ACN0 through ACN63 (0th through 7th order)
- Cross-format conversion (VST3 <-> JUCE <-> CoreAudio <-> AAX)
- Bus layout negotiation framework
- A basic stereo `Panner` class (linear, sin3dB, squareRoot3dB panning rules)

### What Does NOT Exist in JUCE

- **No VBAP** (Vector Base Amplitude Panning) - must implement or use external library
- **No ambisonics encoding/decoding** - must compute spherical harmonics yourself
- **No binaural rendering** - no HRTF convolution
- **No distance attenuation models**
- **No room simulation or reflections**
- **No head tracking integration**
- **No Dolby Atmos object metadata** - JUCE handles beds only, not objects
- **No spatial audio DSP of any kind** beyond the stereo panner

---

## 7. Recommended Architecture for Spatial Granular Synthesis Plugin

### Approach: Ambisonics-First with Decoder Options

The most flexible architecture for a spatial granular synth is:

1. **Internal processing**: Ambisonics (3rd order = 16 channels recommended)
2. **Output modes**: Configurable via `isBusesLayoutSupported()` to output:
   - Stereo (with built-in binaural decoder)
   - Ambisonics 1st-3rd order (for DAWs with ambisonics buses)
   - 7.1.4 Atmos bed (with built-in decoder)
   - 5.1 (with built-in decoder)

### External Libraries for Spatial DSP

| Library | License | Use For |
|---------|---------|---------|
| [SPARTA / Spatial Audio Framework](https://github.com/leomccormack/SPARTA) | GPLv3 | Ambisonics encoding/decoding, VBAP, binaural rendering |
| [libspatialaudio](https://github.com/videolabs/libspatialaudio) | LGPLv2.1 | Ambisonics encoding/decoding |
| [HO-SIRR](https://github.com/leomccormack/HO-SIRR) | BSD | Room impulse response processing |
| [VISR](https://github.com/s3a-spatiern/VISR) | BSD | Versatile interactive scene renderer |

### 1st-Order Ambisonics Encoding (Manual Implementation)

For encoding a mono source at position (azimuth, elevation) into FOA (ACN/SN3D):

```cpp
// azimuth: 0 = front, pi/2 = left, -pi/2 = right, pi = back
// elevation: 0 = horizon, pi/2 = top, -pi/2 = bottom
void encodeToFOA (float sample, float azimuth, float elevation,
                  float& acn0, float& acn1, float& acn2, float& acn3)
{
    float cosElev = std::cos (elevation);

    acn0 = sample;                                         // W (omnidirectional)
    acn1 = sample * std::sin (azimuth) * cosElev;          // Y (left/right)
    acn2 = sample * std::sin (elevation);                  // Z (up/down)
    acn3 = sample * std::cos (azimuth) * cosElev;          // X (front/back)
}
```

### 3rd-Order Ambisonics Encoding (16 channels)

```cpp
void encodeToHOA3 (float sample, float azimuth, float elevation,
                   std::array<float, 16>& ambiChannels)
{
    float cosE = std::cos (elevation);
    float sinE = std::sin (elevation);
    float cosA = std::cos (azimuth);
    float sinA = std::sin (azimuth);
    float cos2A = std::cos (2.0f * azimuth);
    float sin2A = std::sin (2.0f * azimuth);
    float cos3A = std::cos (3.0f * azimuth);
    float sin3A = std::sin (3.0f * azimuth);
    float cosE2 = cosE * cosE;
    float sinE2 = sinE * sinE;

    // Order 0 (ACN 0)
    ambiChannels[0]  = sample;                                                    // W

    // Order 1 (ACN 1-3)
    ambiChannels[1]  = sample * sinA * cosE;                                      // Y
    ambiChannels[2]  = sample * sinE;                                             // Z
    ambiChannels[3]  = sample * cosA * cosE;                                      // X

    // Order 2 (ACN 4-8) - SN3D normalization
    ambiChannels[4]  = sample * std::sqrt(3.0f) * sinA * cosA * cosE2;                       // V
    ambiChannels[5]  = sample * std::sqrt(3.0f) * sinA * sinE * cosE;                        // T
    ambiChannels[6]  = sample * 0.5f * (3.0f * sinE2 - 1.0f);                               // R
    ambiChannels[7]  = sample * std::sqrt(3.0f) * cosA * sinE * cosE;                        // S
    ambiChannels[8]  = sample * std::sqrt(3.0f) * 0.5f * (cosA * cosA - sinA * sinA) * cosE2; // U

    // Order 3 (ACN 9-15) - SN3D normalization
    ambiChannels[9]  = sample * std::sqrt(5.0f/8.0f) * sin3A * cosE * cosE2;
    ambiChannels[10] = sample * std::sqrt(15.0f) * sinA * cosA * cosE2 * sinE;
    ambiChannels[11] = sample * std::sqrt(3.0f/8.0f) * sinA * cosE * (5.0f * sinE2 - 1.0f);
    ambiChannels[12] = sample * 0.5f * sinE * (5.0f * sinE2 - 3.0f);
    ambiChannels[13] = sample * std::sqrt(3.0f/8.0f) * cosA * cosE * (5.0f * sinE2 - 1.0f);
    ambiChannels[14] = sample * std::sqrt(15.0f) * 0.5f * cos2A * cosE2 * sinE;
    ambiChannels[15] = sample * std::sqrt(5.0f/8.0f) * cos3A * cosE * cosE2;
}
```

### Complete CMakeLists.txt Pattern for Multichannel Plugin

```cmake
juce_add_plugin(O-SpatialGrain
    PRODUCT_NAME "O-SpatialGrain"
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Yoco
    PLUGIN_CODE Osgr
    FORMATS VST3 AU Standalone  # AAX only if you don't need 7.1.4
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES "Spatial" "Fx"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    # Note: Do NOT set PLUGIN_CHANNEL_CONFIGURATIONS here.
    # Use isBusesLayoutSupported() for dynamic layout negotiation.
)
```

**Critical**: Do NOT use `PLUGIN_CHANNEL_CONFIGURATIONS` in CMakeLists.txt for multichannel plugins. This legacy option only understands channel counts, not channel types, and will break ambisonics/Atmos layout detection. Always use `isBusesLayoutSupported()` instead.

---

## 8. Practical Recommendations for Spatial Granular Synth

### Target Output Formats (Priority Order)

1. **Stereo** - widest DAW compatibility, use binaural rendering internally
2. **1st Order Ambisonics (4ch)** - works in Reaper, Logic, Nuendo; interoperable with spatial audio chains
3. **3rd Order Ambisonics (16ch)** - high quality, works in Reaper and Nuendo
4. **7.1.4 Atmos (12ch)** - works in Logic 10.7+, Reaper, Nuendo (NOT Pro Tools)
5. **5.1 (6ch)** - legacy surround compatibility

### DAW-Specific Testing Checklist

- **Logic Pro**: Use AU format. Clear AU cache every build. Test on Apple Silicon. Verify with `auval -a`.
- **Reaper**: Use VST3 format. Test channel format switching. Watch for remapping bugs.
- **Nuendo**: Use VST3 format. Best multichannel support. Test ambisonics bus integration.
- **Pro Tools**: Use AAX format. Limited to 7.1.2 max for Atmos. Ambisonics 1st-7th order via ACN stem formats.
- **Ableton Live**: Stereo only. Multichannel will not work.

### VST3 Ambisonics Channel Limit Warning

The older VST3 SDK had a 24-channel speaker arrangement limit. JUCE 8.0.4 bundles an updated SDK with 5th-7th order support, but verify that your target DAWs also support these higher orders. Practically, **3rd order (16 channels) is the safe maximum** for broad DAW compatibility.

---

## Sources

- JUCE Source: `/Users/taylorbrook/JUCE/modules/juce_audio_basics/buffers/juce_AudioChannelSet.h`
- JUCE Source: `/Users/taylorbrook/JUCE/modules/juce_audio_basics/buffers/juce_AudioChannelSet.cpp`
- JUCE Source: `/Users/taylorbrook/JUCE/modules/juce_audio_processors/processors/juce_AudioProcessor.h`
- JUCE Source: `/Users/taylorbrook/JUCE/modules/juce_audio_processors/format_types/juce_VST3Common.h`
- JUCE Source: `/Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_AAX.cpp`
- JUCE Source: `/Users/taylorbrook/JUCE/modules/juce_audio_basics/native/juce_CoreAudioLayouts_mac.h`
- JUCE Source: `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_Panner.h`
- JUCE Forum: Extending Juce support for Atmos beyond 7.1.2 - https://forum.juce.com/t/extending-juce-support-for-atmos-beyond-7-1-2-channel-set-including-in-logic-pro-via-new-layout-tags/48375
- JUCE Forum: VST3 and Ambisonics max 24 channels - https://forum.juce.com/t/vst3-and-ambisonics-in-reaper-mainly-max-24-channels/36185
- JUCE Forum: Does JUCE support Dolby Atmos - https://forum.juce.com/t/does-juce-support-dolby-atmos/35833
- JUCE Forum: Different Multichannel Formats with Reaper - https://forum.juce.com/t/different-multichannel-formats-with-reaper-vst3/56831
- JUCE Forum: Ambisonics 4/5/6/7 broken in VST3.7.8 - https://forum.juce.com/t/ambisonics-4-5-6-7-broken-in-vst3-7-8-reaper-latest/56662
- SPARTA / Spatial Audio Framework - https://github.com/leomccormack/SPARTA
- ambix Plugin Suite - https://github.com/kronihias/ambix
