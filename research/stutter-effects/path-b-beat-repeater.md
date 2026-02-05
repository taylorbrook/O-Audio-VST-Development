---
title: "Multi-Lane Beat Repeater Architecture"
summary: "Design for a polyrhythmic beat repeater with 4 independent repeat lanes, envelope follower triggering, Euclidean rhythm patterns, and tape degradation simulation on repeats."
domain: dsp
type: algorithm
keywords:
  - stutter
  - stutter-effects
  - beat-repeat
  - polyrhythmic
  - euclidean-rhythm
  - tape-degradation
  - multi-lane
stages: [0, 2]
agents: [dsp, research]
---

# Path B: Multi-Lane Beat Repeater

**Simple Buffer Capture with Polyrhythmic Capabilities**

**Estimated Development Time:** 2-3 weeks
**Complexity:** Medium
**Starting Point:** New plugin from scratch

---

## Unique Value Proposition

**"Polyrhythmic Beat Repeater"** - Differentiated by:
- 4 independent repeat lanes with different subdivisions
- Envelope follower triggering (loud = stutter, quiet = pass)
- Sidechain input for external triggering
- Tape degradation simulation on repeats
- Euclidean rhythm patterns per lane

No existing beat repeater offers multi-lane polyrhythmic capabilities. Ableton's Beat Repeat is single-lane. This creates complex evolving patterns from simple input.

---

## Architecture Overview

```
                    ┌─────────────────────────────────────────────────┐
                    │              4-Lane Repeat Engine               │
                    │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐│
Input ──┬──────────▶│  │ Lane 1  │ │ Lane 2  │ │ Lane 3  │ │ Lane 4  ││
        │           │  │  1/8    │ │  1/16   │ │  1/8T   │ │  1/32   ││
        │           │  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘│
        │           │       │           │           │           │     │
        │           │       ▼           ▼           ▼           ▼     │
        │           │  ┌─────────────────────────────────────────┐   │
        │           │  │            Lane Mixer                    │   │
        │           │  └─────────────────────────────────────────┘   │
        │           └─────────────────────┬───────────────────────────┘
        │                                 │
        │                                 ▼
        │                        ┌─────────────────┐
        │                        │  Tape Degrader  │
        │                        │  (per repeat)   │
        │                        └────────┬────────┘
        │                                 │
        ▼                                 ▼
   ┌─────────┐                   ┌─────────────────┐
   │Dry Path │──────────────────▶│   Dry/Wet Mix   │───▶ Output
   └─────────┘                   └─────────────────┘
                                         ▲
                                         │
                               ┌─────────────────┐
                               │ Envelope Follower│
                               │   (Threshold)    │
                               └─────────────────┘
```

---

## Implementation Phases

### Phase 1: Core Infrastructure (3-4 days)

#### 1.1 Project Setup

```cpp
// CMakeLists.txt additions
juce_add_plugin(PolyRepeater
    PLUGIN_MANUFACTURER_CODE Ourc
    PLUGIN_CODE Prpt
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Poly Repeater"
)

target_sources(PolyRepeater PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/RepeatLane.cpp
    Source/RepeatLane.h
    Source/TapeDegrader.cpp
    Source/TapeDegrader.h
)
```

#### 1.2 Capture Buffer Class

```cpp
// CaptureBuffer.h
class CaptureBuffer
{
public:
    void prepare(double sampleRate, int numChannels, float maxCaptureSeconds = 4.0f)
    {
        this->sampleRate = sampleRate;
        int maxSamples = static_cast<int>(sampleRate * maxCaptureSeconds);

        buffer.setSize(numChannels, maxSamples);
        buffer.clear();

        writePosition = 0;
        captureLength = 0;
        isCapturing = false;
    }

    // Continuous capture (rolling buffer)
    void pushSample(int channel, float sample)
    {
        buffer.setSample(channel, writePosition, sample);
        if (channel == buffer.getNumChannels() - 1)
        {
            writePosition = (writePosition + 1) % buffer.getNumSamples();
        }
    }

    // Start capturing specific length
    void startCapture(int lengthInSamples)
    {
        captureStartPosition = writePosition;
        captureLength = juce::jmin(lengthInSamples, buffer.getNumSamples());
        isCapturing = true;
        capturedSamples = 0;
    }

    // Check if capture complete
    bool isCaptureComplete() const
    {
        return capturedSamples >= captureLength;
    }

    // Read from captured segment (for playback)
    float readCapturedSample(int channel, int position) const
    {
        int readPos = (captureStartPosition + position) % buffer.getNumSamples();
        return buffer.getSample(channel, readPos);
    }

    int getCaptureLength() const { return captureLength; }

private:
    juce::AudioBuffer<float> buffer;
    double sampleRate = 44100.0;
    int writePosition = 0;
    int captureStartPosition = 0;
    int captureLength = 0;
    int capturedSamples = 0;
    bool isCapturing = false;
};
```

#### 1.3 Single Repeat Lane

```cpp
// RepeatLane.h
class RepeatLane
{
public:
    enum class Subdivision
    {
        Quarter = 0,
        Eighth,
        Sixteenth,
        ThirtySecond,
        EighthTriplet,
        SixteenthTriplet
    };

    struct Parameters
    {
        bool enabled = true;
        Subdivision subdivision = Subdivision::Sixteenth;
        int repeatCount = 4;
        float decay = 0.9f;          // Amplitude multiplier per repeat
        float pitchShift = 0.0f;     // Semitones per repeat
        float filterSweep = 0.0f;    // -1 to +1 (LP to HP sweep)
        float probability = 1.0f;    // Trigger probability
        float volume = 1.0f;         // Lane output level
    };

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;
        captureBuffer.prepare(sampleRate, 2, 4.0f);

        // Prepare crossfade buffer
        crossfadeLength = static_cast<int>(sampleRate * 0.005);  // 5ms
        crossfadeBuffer.setSize(2, crossfadeLength);

        // Prepare filter
        filterSpec.sampleRate = sampleRate;
        filterSpec.maximumBlockSize = samplesPerBlock;
        filterSpec.numChannels = 2;
        filter.prepare(filterSpec);
    }

    void updateFromHost(double bpm, double ppqPosition, int numSamples)
    {
        currentBpm = bpm;

        // Calculate samples per subdivision
        double samplesPerBeat = (60.0 / bpm) * sampleRate;
        samplesPerSubdiv = calculateSubdivisionSamples(params.subdivision, samplesPerBeat);

        // Check for subdivision boundary crossing
        double ppqPerSample = bpm / (60.0 * sampleRate);
        double subdivInPpq = getSubdivisionInPpq(params.subdivision);

        for (int i = 0; i < numSamples; ++i)
        {
            double currentPpq = ppqPosition + (i * ppqPerSample);
            double prevPpq = currentPpq - ppqPerSample;

            if (crossedSubdivision(prevPpq, currentPpq, subdivInPpq))
            {
                // Probability check
                if (juce::Random::getSystemRandom().nextFloat() <= params.probability)
                {
                    triggerRepeat();
                }
            }
        }
    }

    void processBlock(juce::AudioBuffer<float>& buffer,
                      const juce::AudioBuffer<float>& dryInput)
    {
        if (!params.enabled || state == State::Idle)
        {
            buffer.clear();
            return;
        }

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // Always feed dry input to capture buffer
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                captureBuffer.pushSample(ch, dryInput.getSample(ch, sample));
            }

            if (state == State::Playing)
            {
                processPlayingSample(buffer, sample);
            }
        }

        // Apply filter sweep if active
        if (std::abs(params.filterSweep) > 0.01f)
        {
            applyFilterSweep(buffer);
        }

        // Apply lane volume
        buffer.applyGain(params.volume);
    }

    void triggerRepeat()
    {
        // Start capture of one subdivision length
        captureBuffer.startCapture(static_cast<int>(samplesPerSubdiv));
        currentRepeat = 0;
        repeatPosition = 0;
        currentGain = 1.0f;
        state = State::Playing;
    }

    void setParameters(const Parameters& newParams)
    {
        params = newParams;
    }

private:
    enum class State { Idle, Playing };

    void processPlayingSample(juce::AudioBuffer<float>& buffer, int sample)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float repeatedSample = captureBuffer.readCapturedSample(ch, repeatPosition);

            // Apply decay
            repeatedSample *= currentGain;

            // Apply crossfade at loop boundaries
            if (repeatPosition < crossfadeLength)
            {
                float fadeIn = static_cast<float>(repeatPosition) / crossfadeLength;
                repeatedSample *= fadeIn;
            }
            else if (repeatPosition > captureBuffer.getCaptureLength() - crossfadeLength)
            {
                int fadePos = repeatPosition - (captureBuffer.getCaptureLength() - crossfadeLength);
                float fadeOut = 1.0f - (static_cast<float>(fadePos) / crossfadeLength);
                repeatedSample *= fadeOut;
            }

            buffer.addSample(ch, sample, repeatedSample);
        }

        repeatPosition++;

        // Check for repeat boundary
        if (repeatPosition >= captureBuffer.getCaptureLength())
        {
            repeatPosition = 0;
            currentRepeat++;
            currentGain *= params.decay;

            // Apply pitch shift for next repeat
            // (simplified - would need proper pitch shifting)

            if (currentRepeat >= params.repeatCount)
            {
                state = State::Idle;
            }
        }
    }

    void applyFilterSweep(juce::AudioBuffer<float>& buffer)
    {
        // Sweep from current repeat position
        float sweepProgress = static_cast<float>(currentRepeat) / params.repeatCount;
        float cutoff;

        if (params.filterSweep > 0)
        {
            // High-pass sweep (starts open, closes)
            cutoff = 20.0f + sweepProgress * params.filterSweep * 10000.0f;
            *filter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, cutoff, 0.707f);
        }
        else
        {
            // Low-pass sweep (starts open, closes)
            cutoff = 20000.0f - sweepProgress * std::abs(params.filterSweep) * 19000.0f;
            *filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                sampleRate, cutoff, 0.707f);
        }

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        filter.process(context);
    }

    double calculateSubdivisionSamples(Subdivision sub, double samplesPerBeat)
    {
        switch (sub)
        {
            case Subdivision::Quarter:         return samplesPerBeat;
            case Subdivision::Eighth:          return samplesPerBeat / 2.0;
            case Subdivision::Sixteenth:       return samplesPerBeat / 4.0;
            case Subdivision::ThirtySecond:    return samplesPerBeat / 8.0;
            case Subdivision::EighthTriplet:   return samplesPerBeat / 3.0;
            case Subdivision::SixteenthTriplet: return samplesPerBeat / 6.0;
            default: return samplesPerBeat / 4.0;
        }
    }

    double getSubdivisionInPpq(Subdivision sub)
    {
        switch (sub)
        {
            case Subdivision::Quarter:         return 1.0;
            case Subdivision::Eighth:          return 0.5;
            case Subdivision::Sixteenth:       return 0.25;
            case Subdivision::ThirtySecond:    return 0.125;
            case Subdivision::EighthTriplet:   return 1.0 / 3.0;
            case Subdivision::SixteenthTriplet: return 1.0 / 6.0;
            default: return 0.25;
        }
    }

    bool crossedSubdivision(double oldPpq, double newPpq, double subdivPpq)
    {
        return std::floor(newPpq / subdivPpq) > std::floor(oldPpq / subdivPpq);
    }

    Parameters params;
    State state = State::Idle;

    CaptureBuffer captureBuffer;
    juce::AudioBuffer<float> crossfadeBuffer;

    double sampleRate = 44100.0;
    double currentBpm = 120.0;
    double samplesPerSubdiv = 5512.5;

    int repeatPosition = 0;
    int currentRepeat = 0;
    float currentGain = 1.0f;
    int crossfadeLength = 220;

    juce::dsp::ProcessSpec filterSpec;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>> filter;
};
```

---

### Phase 2: Multi-Lane Engine (2-3 days)

#### 2.1 Four-Lane Processor

```cpp
// PluginProcessor.h
class PolyRepeaterProcessor : public juce::AudioProcessor
{
public:
    static constexpr int NUM_LANES = 4;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        for (auto& lane : lanes)
        {
            lane.prepare(sampleRate, samplesPerBlock);
        }

        // Prepare mixing buffer
        laneOutputBuffer.setSize(2, samplesPerBlock);
        dryBuffer.setSize(2, samplesPerBlock);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;

        // Get tempo info
        auto tempoInfo = getTempoInfo();

        // Store dry signal
        dryBuffer.makeCopyOf(buffer);

        // Clear output for accumulation
        buffer.clear();

        // Process each lane
        for (int laneIdx = 0; laneIdx < NUM_LANES; ++laneIdx)
        {
            if (!lanes[laneIdx].isEnabled()) continue;

            laneOutputBuffer.clear();

            lanes[laneIdx].updateFromHost(tempoInfo.bpm, tempoInfo.ppqPosition,
                                          buffer.getNumSamples());
            lanes[laneIdx].processBlock(laneOutputBuffer, dryBuffer);

            // Mix lane output to main buffer
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                buffer.addFrom(ch, 0, laneOutputBuffer, ch, 0,
                              buffer.getNumSamples());
            }
        }

        // Mix dry signal based on overall wet level
        float wetLevel = *parameters.getRawParameterValue("mix") / 100.0f;
        float dryLevel = 1.0f - wetLevel;

        buffer.applyGain(wetLevel);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.addFrom(ch, 0, dryBuffer, ch, 0, buffer.getNumSamples(), dryLevel);
        }
    }

private:
    std::array<RepeatLane, NUM_LANES> lanes;
    juce::AudioBuffer<float> laneOutputBuffer;
    juce::AudioBuffer<float> dryBuffer;
};
```

#### 2.2 Per-Lane Parameters

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Global parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix", 0.0f, 100.0f, 50.0f));

    // Per-lane parameters (Lane 1-4)
    for (int lane = 0; lane < 4; ++lane)
    {
        juce::String prefix = "lane" + juce::String(lane + 1) + "_";

        layout.add(std::make_unique<juce::AudioParameterBool>(
            prefix + "enabled", "Lane " + juce::String(lane + 1) + " Enabled",
            lane == 0));  // Only lane 1 enabled by default

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            prefix + "subdivision", "Subdivision",
            juce::StringArray { "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T" },
            2));  // Default: 1/16

        layout.add(std::make_unique<juce::AudioParameterInt>(
            prefix + "repeats", "Repeats", 1, 16, 4));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "decay", "Decay", 0.0f, 100.0f, 90.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "pitch", "Pitch", -12.0f, 12.0f, 0.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "filter", "Filter", -100.0f, 100.0f, 0.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "probability", "Probability", 0.0f, 100.0f, 100.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "volume", "Volume", 0.0f, 100.0f, 100.0f));
    }

    return layout;
}
```

---

### Phase 3: Unique Features (3-4 days)

#### 3.1 Tape Degradation Simulator

```cpp
// TapeDegrader.h
class TapeDegrader
{
public:
    struct Settings
    {
        float saturationAmount = 0.0f;   // 0-1
        float wowFlutterDepth = 0.0f;    // 0-1
        float wowFlutterRate = 2.0f;     // Hz
        float hissLevel = 0.0f;          // 0-1
        float hfRolloff = 0.0f;          // 0-1, how much HF loss per repeat
        float dropoutChance = 0.0f;      // 0-1, probability of momentary dropout
    };

    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;

        // Prepare wow/flutter LFO
        wowPhase = 0.0;
        flutterPhase = 0.0;

        // Prepare HF rolloff filter
        rolloffFilter.prepare({ sampleRate, 512, 2 });

        // Prepare saturation
        saturator.prepare({ sampleRate, 512, 2 });
    }

    void processRepeat(juce::AudioBuffer<float>& buffer, int repeatNumber,
                       const Settings& settings)
    {
        // Degradation increases with each repeat
        float degradationFactor = static_cast<float>(repeatNumber) / 8.0f;
        degradationFactor = juce::jmin(degradationFactor, 1.0f);

        // 1. Apply wow and flutter (pitch modulation)
        if (settings.wowFlutterDepth > 0.01f)
        {
            applyWowFlutter(buffer, settings.wowFlutterDepth * degradationFactor,
                           settings.wowFlutterRate);
        }

        // 2. Apply tape saturation
        if (settings.saturationAmount > 0.01f)
        {
            applySaturation(buffer, settings.saturationAmount * degradationFactor);
        }

        // 3. Apply HF rolloff (cumulative with repeats)
        if (settings.hfRolloff > 0.01f)
        {
            float cutoff = 20000.0f * std::pow(1.0f - settings.hfRolloff, repeatNumber);
            cutoff = juce::jmax(cutoff, 500.0f);
            applyHFRolloff(buffer, cutoff);
        }

        // 4. Add tape hiss
        if (settings.hissLevel > 0.001f)
        {
            addTapeHiss(buffer, settings.hissLevel * 0.1f);
        }

        // 5. Random dropouts
        if (settings.dropoutChance > 0.0f)
        {
            applyDropouts(buffer, settings.dropoutChance * degradationFactor);
        }
    }

private:
    void applyWowFlutter(juce::AudioBuffer<float>& buffer, float depth, float rate)
    {
        // Simple pitch modulation via sample skipping/repeating
        double wowIncrement = rate * 2.0 * juce::MathConstants<double>::pi / sampleRate;
        double flutterIncrement = rate * 12.0 * juce::MathConstants<double>::pi / sampleRate;

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // Combined wow (slow) and flutter (fast)
            float modulation = std::sin(wowPhase) * 0.7f + std::sin(flutterPhase) * 0.3f;
            modulation *= depth * 0.01f;  // Max ±1% pitch variation

            // Apply subtle pitch shift by interpolating
            // (Simplified - real implementation would use proper resampling)

            wowPhase += wowIncrement;
            flutterPhase += flutterIncrement;
        }
    }

    void applySaturation(juce::AudioBuffer<float>& buffer, float amount)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                // Soft clip with tape-like asymmetry
                float x = data[i] * (1.0f + amount * 2.0f);
                data[i] = std::tanh(x) * (1.0f / std::tanh(1.0f + amount * 2.0f));
            }
        }
    }

    void applyHFRolloff(juce::AudioBuffer<float>& buffer, float cutoffHz)
    {
        *rolloffFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate, cutoffHz, 0.707f);

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        rolloffFilter.process(context);
    }

    void addTapeHiss(juce::AudioBuffer<float>& buffer, float level)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
                data[i] += noise * level;
            }
        }
    }

    void applyDropouts(juce::AudioBuffer<float>& buffer, float chance)
    {
        // Random brief silences
        for (int i = 0; i < buffer.getNumSamples(); i += 64)
        {
            if (juce::Random::getSystemRandom().nextFloat() < chance * 0.01f)
            {
                int dropoutLength = juce::Random::getSystemRandom().nextInt({ 16, 64 });
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                {
                    for (int j = i; j < juce::jmin(i + dropoutLength, buffer.getNumSamples()); ++j)
                    {
                        buffer.setSample(ch, j, 0.0f);
                    }
                }
            }
        }
    }

    double sampleRate = 44100.0;
    double wowPhase = 0.0;
    double flutterPhase = 0.0;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>> rolloffFilter;
    juce::dsp::Gain<float> saturator;
};
```

#### 3.2 Envelope Follower Trigger

```cpp
// EnvelopeFollower.h
class EnvelopeFollower
{
public:
    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
        envelope = 0.0f;
    }

    void setParameters(float attackMs, float releaseMs, float threshold)
    {
        attackCoeff = std::exp(-1.0f / (sampleRate * attackMs / 1000.0f));
        releaseCoeff = std::exp(-1.0f / (sampleRate * releaseMs / 1000.0f));
        this->threshold = threshold;
    }

    // Returns true if envelope crossed threshold (upward)
    bool process(float inputSample)
    {
        float inputAbs = std::abs(inputSample);

        bool previouslyAbove = envelope > threshold;

        // Attack/release envelope following
        if (inputAbs > envelope)
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * inputAbs;
        else
            envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * inputAbs;

        bool nowAbove = envelope > threshold;

        // Detect upward threshold crossing
        return nowAbove && !previouslyAbove;
    }

    float getEnvelope() const { return envelope; }

private:
    double sampleRate = 44100.0;
    float envelope = 0.0f;
    float attackCoeff = 0.99f;
    float releaseCoeff = 0.9999f;
    float threshold = 0.5f;
};

// Usage in processor
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
    // Check envelope follower for each sample
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float inputLevel = std::abs(buffer.getSample(0, sample));

        if (envelopeFollower.process(inputLevel))
        {
            // Trigger repeats when input exceeds threshold
            for (auto& lane : lanes)
            {
                if (lane.isEnvelopeTriggered())
                {
                    lane.triggerRepeat();
                }
            }
        }
    }

    // ... rest of processing
}
```

#### 3.3 Sidechain Input

```cpp
// In PluginProcessor
PolyRepeaterProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)  // Optional SC
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

bool isBusesLayoutSupported(const BusesLayout& layouts) const override
{
    // Main I/O must be stereo
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Sidechain is optional, but if present must be stereo or mono
    auto scChannels = layouts.getChannelSet(true, 1);
    if (!scChannels.isDisabled() &&
        scChannels != juce::AudioChannelSet::stereo() &&
        scChannels != juce::AudioChannelSet::mono())
        return false;

    return true;
}

void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
    auto mainInput = getBusBuffer(buffer, true, 0);
    auto sidechainInput = getBusBuffer(buffer, true, 1);

    bool hasSidechain = sidechainInput.getNumChannels() > 0;

    // Use sidechain for triggering if available
    const juce::AudioBuffer<float>& triggerSource =
        hasSidechain ? sidechainInput : mainInput;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float triggerLevel = std::abs(triggerSource.getSample(0, sample));

        if (envelopeFollower.process(triggerLevel))
        {
            // Sidechain triggered repeat
            for (auto& lane : lanes)
            {
                if (lane.isSidechainTriggered())
                {
                    lane.triggerRepeat();
                }
            }
        }
    }
}
```

---

### Phase 4: UI Design (2-3 days)

#### 4.1 WebView Layout

```html
<!DOCTYPE html>
<html>
<head>
    <style>
        body {
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            color: #fff;
            font-family: 'Segoe UI', sans-serif;
            margin: 0;
            padding: 20px;
        }

        .lanes-container {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 15px;
            margin-bottom: 20px;
        }

        .lane {
            background: rgba(255,255,255,0.05);
            border-radius: 10px;
            padding: 15px;
            border: 2px solid transparent;
            transition: border-color 0.2s;
        }

        .lane.active {
            border-color: #00ffff;
        }

        .lane.triggered {
            animation: pulse 0.1s ease-out;
        }

        @keyframes pulse {
            0% { background: rgba(0,255,255,0.3); }
            100% { background: rgba(255,255,255,0.05); }
        }

        .lane-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 10px;
        }

        .lane-title {
            font-size: 14px;
            font-weight: bold;
        }

        .toggle-btn {
            width: 40px;
            height: 20px;
            background: #333;
            border-radius: 10px;
            cursor: pointer;
            position: relative;
        }

        .toggle-btn.on {
            background: #00ffff;
        }

        .toggle-btn::after {
            content: '';
            position: absolute;
            width: 16px;
            height: 16px;
            background: white;
            border-radius: 50%;
            top: 2px;
            left: 2px;
            transition: left 0.2s;
        }

        .toggle-btn.on::after {
            left: 22px;
        }

        .subdivision-select {
            width: 100%;
            background: #222;
            color: #fff;
            border: 1px solid #444;
            padding: 5px;
            border-radius: 5px;
            margin-bottom: 10px;
        }

        .knob-row {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 10px;
        }

        .knob-container {
            text-align: center;
        }

        .knob {
            width: 40px;
            height: 40px;
            background: radial-gradient(circle at 30% 30%, #444, #222);
            border-radius: 50%;
            margin: 0 auto 5px;
            cursor: pointer;
            position: relative;
        }

        .knob-label {
            font-size: 10px;
            opacity: 0.7;
        }

        /* Global section */
        .global-section {
            display: flex;
            gap: 20px;
            align-items: center;
            padding: 15px;
            background: rgba(0,0,0,0.3);
            border-radius: 10px;
        }

        .mix-slider {
            flex: 1;
            height: 10px;
            background: #333;
            border-radius: 5px;
            cursor: pointer;
        }

        /* Tape degradation */
        .tape-section {
            margin-top: 20px;
            padding: 15px;
            background: rgba(139,69,19,0.2);
            border-radius: 10px;
            border: 1px solid rgba(139,69,19,0.5);
        }

        .tape-title {
            font-size: 12px;
            opacity: 0.7;
            margin-bottom: 10px;
        }

        .tape-knobs {
            display: grid;
            grid-template-columns: repeat(6, 1fr);
            gap: 10px;
        }
    </style>
</head>
<body>
    <div class="lanes-container">
        <!-- Lane 1 -->
        <div class="lane active" id="lane1">
            <div class="lane-header">
                <span class="lane-title">LANE 1</span>
                <div class="toggle-btn on" data-param="lane1_enabled"></div>
            </div>
            <select class="subdivision-select" data-param="lane1_subdivision">
                <option value="0">1/4</option>
                <option value="1">1/8</option>
                <option value="2" selected>1/16</option>
                <option value="3">1/32</option>
                <option value="4">1/8T</option>
                <option value="5">1/16T</option>
            </select>
            <div class="knob-row">
                <div class="knob-container">
                    <div class="knob" data-param="lane1_repeats"></div>
                    <div class="knob-label">REPS</div>
                </div>
                <div class="knob-container">
                    <div class="knob" data-param="lane1_decay"></div>
                    <div class="knob-label">DECAY</div>
                </div>
                <div class="knob-container">
                    <div class="knob" data-param="lane1_probability"></div>
                    <div class="knob-label">PROB</div>
                </div>
            </div>
            <div class="knob-row" style="margin-top: 10px;">
                <div class="knob-container">
                    <div class="knob" data-param="lane1_pitch"></div>
                    <div class="knob-label">PITCH</div>
                </div>
                <div class="knob-container">
                    <div class="knob" data-param="lane1_filter"></div>
                    <div class="knob-label">FILTER</div>
                </div>
                <div class="knob-container">
                    <div class="knob" data-param="lane1_volume"></div>
                    <div class="knob-label">VOL</div>
                </div>
            </div>
        </div>

        <!-- Lane 2-4 similar structure -->
        <div class="lane" id="lane2"><!-- ... --></div>
        <div class="lane" id="lane3"><!-- ... --></div>
        <div class="lane" id="lane4"><!-- ... --></div>
    </div>

    <div class="tape-section">
        <div class="tape-title">TAPE DEGRADATION</div>
        <div class="tape-knobs">
            <div class="knob-container">
                <div class="knob" data-param="tape_saturation"></div>
                <div class="knob-label">SAT</div>
            </div>
            <div class="knob-container">
                <div class="knob" data-param="tape_wow"></div>
                <div class="knob-label">WOW</div>
            </div>
            <div class="knob-container">
                <div class="knob" data-param="tape_flutter"></div>
                <div class="knob-label">FLUTTER</div>
            </div>
            <div class="knob-container">
                <div class="knob" data-param="tape_hiss"></div>
                <div class="knob-label">HISS</div>
            </div>
            <div class="knob-container">
                <div class="knob" data-param="tape_rolloff"></div>
                <div class="knob-label">ROLLOFF</div>
            </div>
            <div class="knob-container">
                <div class="knob" data-param="tape_dropout"></div>
                <div class="knob-label">DROPOUT</div>
            </div>
        </div>
    </div>

    <div class="global-section">
        <span>DRY</span>
        <div class="mix-slider" data-param="mix"></div>
        <span>WET</span>

        <div class="toggle-btn" data-param="envelope_trigger">ENV</div>
        <div class="toggle-btn" data-param="sidechain_trigger">SC</div>
    </div>
</body>
</html>
```

---

## Testing Checklist

### Per-Lane Tests
- [ ] Each lane can be enabled/disabled independently
- [ ] Subdivision timing is accurate for all options
- [ ] Triplet timing is correct
- [ ] Repeat count works (1-16 repeats)
- [ ] Decay reduces volume correctly per repeat
- [ ] Pitch shift works (±12 semitones)
- [ ] Filter sweep opens/closes with repeats
- [ ] Probability gates triggers correctly
- [ ] Volume per lane works

### Multi-Lane Tests
- [ ] 4 lanes can run simultaneously
- [ ] Different subdivisions create polyrhythms
- [ ] CPU usage acceptable with all 4 lanes active
- [ ] Lane mixing doesn't clip

### Unique Features Tests
- [ ] Tape saturation adds warmth without distortion
- [ ] Wow/flutter creates subtle pitch modulation
- [ ] HF rolloff reduces highs progressively
- [ ] Tape hiss is subtle and natural
- [ ] Dropouts occur randomly at correct probability
- [ ] Envelope follower triggers on transients
- [ ] Threshold sensitivity works
- [ ] Sidechain input triggers repeats on external signal

### Edge Cases
- [ ] Very fast subdivisions (1/32 at 180 BPM) work
- [ ] Very slow subdivisions (1/4 at 60 BPM) work
- [ ] Tempo changes mid-playback handled smoothly
- [ ] No clicks at repeat boundaries (crossfade works)
- [ ] Offline rendering matches real-time

---

## File Structure

```
PolyRepeater/
├── CMakeLists.txt
├── Source/
│   ├── PluginProcessor.cpp
│   ├── PluginProcessor.h
│   ├── PluginEditor.cpp
│   ├── PluginEditor.h
│   ├── RepeatLane.cpp
│   ├── RepeatLane.h
│   ├── CaptureBuffer.h
│   ├── TapeDegrader.cpp
│   ├── TapeDegrader.h
│   ├── EnvelopeFollower.h
│   └── ui/
│       └── public/
│           ├── index.html
│           ├── main.js
│           └── styles.css
├── Presets/
│   ├── Default.txt
│   ├── Polyrhythmic.txt
│   ├── TapeLoop.txt
│   └── SidechainPump.txt
└── .ideas/
    ├── creative-brief.md
    ├── architecture.md
    ├── parameter-spec.md
    └── plan.md
```

---

## Future Enhancements

1. **Pattern Sequencer:** 16-step on/off pattern per lane
2. **Swing Control:** Timing offset for groove
3. **Reverse Mode:** Captured segment plays backward
4. **Ping-Pong Pan:** Repeats alternate L/R
5. **MIDI Trigger:** Specific notes trigger specific lanes
6. **Freeze Mode:** Hold current capture indefinitely
7. **Modulation Matrix:** LFO → any parameter
8. **Preset Morphing:** Crossfade between two preset states
